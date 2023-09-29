#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "async.h"


#define __ASYNC_FINISHED    0x1
#define __ASYNC_DELETE      0x2
#define __ASYNC_ENTERED     0x4

#define handle_error() ({printf("Error %s\n", strerror(errno)); exit(-1);})

/** Main coroutine structure, its context. */
struct coro {
    /** A value, returned by func. */
    void *ret;
    /** Stack, used by the coroutine. */
    void *stack;
    /** An argument for the function func. */
    void *func_arg;
    /** A function to call as a coroutine. */
    async_f func;
    /** Last remembered coroutine context. */
    sigjmp_buf ctx;
    /** True, if the coroutine has finished. */
    int8_t flag;
    /** Links in the coroutine list, used by scheduler. */
    struct coro *next, *prev;
};


static struct coro coro_sched;
static struct coro *coro_this_ptr = NULL;
static struct coro *coro_list = NULL;
static struct coro *coro_list_last = NULL;
static sigjmp_buf start_point;
int8_t is_waiting = 0;
int8_t is_first = 1;


void coro_sched_init(void) {
    memset(&coro_sched, 0, sizeof(coro_sched));
    coro_this_ptr = &coro_sched;
    is_first = 0;
}

static void coro_list_add(struct coro *c) {
    if (coro_list == NULL) coro_list = c;
    c->next = NULL;
    c->prev = coro_list_last;
    if (coro_list_last != NULL)
        coro_list_last->next = c;
    coro_list_last = c;
}

static void coro_list_delete(struct coro *c) {
    struct coro *prev = c->prev, *next = c->next;
    if (prev != NULL) prev->next = next;
    else coro_list = next;
    if (next != NULL) next->prev = prev;
    else coro_list_last = prev;
}

void coro_delete(struct coro *c) {
    free(c->stack);
    free(c);
}

static void coro_yield_to(struct coro *to) {
    struct coro *from = coro_this_ptr;
    int flag = (to->flag & __ASYNC_ENTERED);
    to->flag |= __ASYNC_ENTERED;

    if (sigsetjmp(from->ctx, 0) == 0)
        siglongjmp(to->ctx, 1);

    if (!flag) {
        if ((to->flag & __ASYNC_FINISHED)) {
            coro_list_delete(to);
            if ((to->flag & __ASYNC_DELETE)) {
                coro_delete(to);
            }
        } else
            to->flag ^= __ASYNC_ENTERED;
    }
    coro_this_ptr = from;
}

static void coro_body(int signum) {
    struct coro *c = coro_this_ptr;
    coro_this_ptr = NULL;

    if (sigsetjmp(c->ctx, 0) == 0)
        siglongjmp(start_point, 1);

    coro_this_ptr = c;
    c->ret = c->func(c->func_arg);

    c->flag |= __ASYNC_FINISHED;
    siglongjmp(coro_sched.ctx, 1);
}

struct coro *coro_new(async_f func, void *func_arg, int8_t flag) {
    struct coro *c = (struct coro *) malloc(sizeof(*c));
    c->ret = 0;
    c->stack = malloc(SIGSTKSZ);
    c->func = func;
    c->func_arg = func_arg;
    c->flag = flag;
    /*
     * SIGUSR2 is used. First of all, block new signals to be
     * able to set a new handler.
     */
    sigset_t news, olds, suss;
    sigemptyset(&news);
    sigaddset(&news, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &news, &olds) != 0)
        handle_error();
    /*
     * New handler should jump onto a new stack and remember
     * that position. Afterwards the stack is disabled and
     * becomes dedicated to that single coroutine.
     */
    struct sigaction newsa, oldsa;
    newsa.sa_handler = coro_body;
    newsa.sa_flags = SA_ONSTACK;
    sigemptyset(&newsa.sa_mask);
    if (sigaction(SIGUSR2, &newsa, &oldsa) != 0)
        handle_error();
    /* Create that new stack. */
    stack_t oldst, newst;
    newst.ss_sp = c->stack;
    newst.ss_size = SIGSTKSZ;
    newst.ss_flags = 0;
    if (sigaltstack(&newst, &oldst) != 0)
        handle_error();
    /* Jump onto the stack and remember its position. */
    struct coro *old_this = coro_this_ptr;
    coro_this_ptr = c;
    sigemptyset(&suss);
    if (sigsetjmp(start_point, 1) == 0) {
        raise(SIGUSR2);
        while (coro_this_ptr != NULL)
            sigsuspend(&suss);
    }
    coro_this_ptr = old_this;
    /*
     * Return the old stack, unblock SIGUSR2. In other words,
     * rollback all global changes. The newly created stack
     * now is remembered only by the new coroutine, and can be
     * used by it only.
     */
    if (sigaltstack(NULL, &newst) != 0)
        handle_error();
    newst.ss_flags = SS_DISABLE;
    if (sigaltstack(&newst, NULL) != 0)
        handle_error();
    if ((oldst.ss_flags & SS_DISABLE) == 0 &&
        sigaltstack(&oldst, NULL) != 0)
        handle_error();
    if (sigaction(SIGUSR2, &oldsa, NULL) != 0)
        handle_error();
    if (sigprocmask(SIG_SETMASK, &olds, NULL) != 0)
        handle_error();

    /* Now scheduler can work with that coroutine. */
    coro_list_add(c);
    return c;
}


void *__await_func(async_f func, void *func_arg) {
    if (is_first) coro_sched_init();
    struct coro *c = coro_new(func, func_arg, 0);
    is_waiting = 1;
    while(!(c->flag & __ASYNC_FINISHED))
        async_yield();

    void *ret = c->ret;
    coro_delete(c);
    return ret;
}
void __async_func(async_f func, void *func_arg) {
    if (is_first) coro_sched_init();
    coro_new(func, func_arg, __ASYNC_DELETE);
}

void __async_yield(void) {
    if (is_first) coro_sched_init();
    struct coro *from = coro_this_ptr;
    struct coro *to = from->next;

    /*
     * Here we check that timer for current coroutine work more than one time_quant
     * and adding this time period to the delta time of current coroutine.
     * delta time - is the period of time witch this coroutine worked.
     */

    if (to == NULL)
        if (is_waiting)
            coro_yield_to(coro_list);
        else
            coro_yield_to(&coro_sched);
    else
        coro_yield_to(to);
}
void __async_wait_all(void) {
    if (is_first) coro_sched_init();
    while (coro_list != NULL)
        coro_yield_to(coro_list);
}