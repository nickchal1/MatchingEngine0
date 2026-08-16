cmake_minimum_required(VERSION 3.20)

project(MatchingEngine LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Shared warning settings for our own targets.
add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
)

# Core order-book library.
add_library(orderbook STATIC
    OrderBook.cpp
)

target_compile_features(orderbook PUBLIC
    cxx_std_20
)

target_include_directories(orderbook PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orderbook PRIVATE
    project_warnings
)

# Demo executable.
add_executable(matching_engine
    main.cpp
)

target_link_libraries(matching_engine PRIVATE
    orderbook
    project_warnings
)

add_executable(latency_benchmark
    benchmarks/OrderBookBenchmark.cpp
)

target_link_libraries(latency_benchmark PRIVATE
    orderbook
    project_warnings
)

# Testing support.
include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(orderbook_tests
        tests/OrderBookTests.cpp
    )

    target_link_libraries(orderbook_tests PRIVATE
        orderbook
        project_warnings
        GTest::gtest_main
    )

    include(GoogleTest)

    gtest_discover_tests(orderbook_tests)
endif()