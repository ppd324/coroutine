cmake_minimum_required(VERSION 3.16)
project(coroutine C)

set(CMAKE_C_STANDARD 99)

add_executable(coroutine main.c libco.h libco.c)