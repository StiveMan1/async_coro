# Stackful Coroutine Runtime in C

Built a stackful coroutine runtime in C with cooperative scheduling and manual context switching, enabling lightweight task execution without relying on OS threads.

## Overview

This project implements a small custom coroutine scheduler in pure C. Each coroutine runs on its own private stack and yields explicitly with `async_yield()`, allowing other coroutines to run in round-robin order.

The runtime keeps execution cooperative: a coroutine continues running until it returns or calls `async_yield()`.

## API

* `async_func(func, arg)`: start a coroutine and continue without waiting for it.
* `await_func(func, arg)`: start a coroutine, wait for it to finish, and return its result.
* `async_yield()`: yield from the current coroutine to the scheduler.
* `async_wait_all()`: run scheduled coroutines until all have completed.

## Example

```c
#include "async.h"

#include <stdio.h>

static void *task(void *arg) {
    const char *name = arg;

    printf("%s: start\n", name);
    async_yield();
    printf("%s: resumed\n", name);

    return NULL;
}

int main(void) {
    async_func(task, "first");
    async_func(task, "second");
    async_wait_all();

    return 0;
}
```

Example output:

```text
first: start
second: start
first: resumed
second: resumed
```

## Notes

* No pthreads or OS worker threads are used.
* Scheduling is cooperative, not preemptive.
* The implementation uses separate coroutine stacks with `sigsetjmp` / `siglongjmp`.
