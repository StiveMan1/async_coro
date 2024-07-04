# Asynchronous Function Library
This library provides a simple framework for asynchronous function execution in C. It allows functions to be executed concurrently and provides mechanisms for yielding execution, waiting for all asynchronous functions to complete, and awaiting specific asynchronous function results.

## Features
* `__await_func(func, arg)`: Initiates an asynchronous function call and waits for its completion, returning the result.
* `__async_func(func, arg)`: Initiates an asynchronous function call without waiting for its completion.
* `__async_yield()`: Yields the current asynchronous function's execution, allowing other tasks to proceed.
* `__async_wait_all()`: Waits for all asynchronously initiated tasks to complete before returning.

## Usage
To use this library, include `async.h` in your C source files. Here's a brief overview of the provided macros:

* `await_func(func, arg)`: Macro for `__await_func(func, arg)`.
* `async_func(func, arg)`: Macro for `__async_func(func, arg)`.
* `async_yield`: Macro for `__async_yield()`.
* `async_wait_all`: Macro for `__async_wait_all()`.
## Example
```c++
#include "async.h"
#include <stdio.h>
#include <pthread.h>

void *async_task(void *arg) {
int *num = (int *)arg;
printf("Async task started with argument: %d\n", *num);
async_yield();  // Yield execution
printf("Async task resumed with argument: %d\n", *num);
return NULL;
}

int main() {
int num = 10;

    // Asynchronously execute async_task
    async_func(async_task, &num);
    
    // Wait for all asynchronous tasks to complete
    async_wait_all();
    
    printf("All asynchronous tasks completed.\n");
    
    return 0;
}
```
## Notes
* This library utilizes POSIX threads (`pthread.h`) for managing asynchronous tasks.
* Ensure proper synchronization and error handling in real-world applications.
* Always include async.h and link with `-lpthread` when compiling.