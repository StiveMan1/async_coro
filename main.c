#include "async.h"

#include <stdio.h>

static void *worker(void *arg) {
    const char *name = arg;

    printf("%s: start\n", name);
    async_yield();
    printf("%s: resumed\n", name);

    return NULL;
}

static void *return_value(void *arg) {
    printf("awaited: start\n");
    async_yield();
    printf("awaited: returning\n");

    return arg;
}

int main(void) {
    int value = 42;
    int *result;

    async_func(worker, "first");
    async_func(worker, "second");
    async_wait_all();

    result = await_func(return_value, &value);
    printf("awaited result: %d\n", *result);

    return 0;
}
