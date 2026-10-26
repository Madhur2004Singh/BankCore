cmake_minimum_required(VERSION 3.16)
project(BankCoreLite VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(bankcore
    src/Bank.cpp
    src/ThreadPool.cpp
)
target_include_directories(bankcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bankcore PUBLIC Threads::Threads)
target_compile_options(bankcore PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bank_demo src/main.cpp)
target_link_libraries(bank_demo PRIVATE bankcore)

add_executable(bank_tests tests/bank_tests.cpp)
target_link_libraries(bank_tests PRIVATE bankcore)

add_executable(bank_benchmark benchmarks/benchmark.cpp)
target_link_libraries(bank_benchmark PRIVATE bankcore)

enable_testing()
add_test(NAME bank_tests COMMAND bank_tests)
