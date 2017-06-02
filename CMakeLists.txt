cmake_minimum_required(VERSION 3.7)
project(num_words)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -O1)

set(SOURCE_FILES
        num_words/num_words.cpp)

add_executable(num_words ${SOURCE_FILES})
add_executable(gen num_words/gen.cpp)