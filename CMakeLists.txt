cmake_minimum_required(VERSION 3.10)

# set the project name
project(coro)
# specify the C++ standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++2a -stdlib=libc++")

# Add the executable

add_executable(coro src/coro.cpp)
add_executable(fizzbuzz src/fizz_buzz.cpp)
add_executable(coawait src/coawait.cpp)
add_executable(coawaiters src/co_awaiters.cpp)
add_executable(fizz_coawait src/fizz_coawait.cpp)
add_executable(coro_fizz src/coro_fizz.cpp)

target_compile_options(coro PRIVATE -fcoroutines-ts)
