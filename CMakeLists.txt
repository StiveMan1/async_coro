cmake_minimum_required(VERSION 3.26)
project(async C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(async main.c async.c)

target_compile_options(async PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -pedantic>
)
