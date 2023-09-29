#ifndef ASYNC_H
#define ASYNC_H

typedef void *(*async_f)(void *);

void *__await_func(async_f func, void *arg);
void __async_func(async_f func, void *arg);

void __async_yield(void);
void __async_wait_all(void);

#define await_func __await_func
#define async_func __async_func

#define async_yield __async_yield
#define async_wait_all __async_wait_all

#endif //ASYNC_H
