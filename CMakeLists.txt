cmake_minimum_required(VERSION 3.7)
project(num_words)

set(CMAKE_CXX_STANDARD 11)

set(NUM_WORDS
        num_words/num_words.cpp)

add_executable(memcpy memcpy/memcpy.cpp)

target_compile_options(memcpy PUBLIC -O3)

add_executable(num_words ${NUM_WORDS})

target_compile_options(num_words PUBLIC -mssse3)
