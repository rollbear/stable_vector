cmake_minimum_required(VERSION 3.16)
project(stable_vector)

set(CMAKE_CXX_STANDARD 20)

option(FUZZER OFF "Build a fuzz-tester")

set(MASTER_PROJECT OFF)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(MASTER_PROJECT ON)
endif()

add_library(stable_vector INTERFACE)
add_library(stable_vector::stable_vector ALIAS stable_vector)
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(stable_vector INTERFACE $<BUILD_INTERFACE:${INCLUDE_DIR}>)
target_include_directories(stable_vector INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

install(TARGETS stable_vector EXPORT stable_vector-targets INCLUDES DESTINATION include)
install(EXPORT stable_vector-targets DESTINATION lib/cmake/stable_vector)
install(FILES stable_vector-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/stable_vector/stable_vector-config-version.cmake
        DESTINATION lib/cmake/stable_vector COMPONENT Devel)
install(DIRECTORY "include" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

if (MASTER_PROJECT AND CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_STANDARD_REQUIRED YES)
    set(CMAKE_CXX_EXTENSIONS OFF)

    find_package(Catch2 3 QUIET)
    if (TARGET Catch2::Catch2WithMain)
        add_executable(test_stable_vector main.cpp)

        target_link_libraries(test_stable_vector PRIVATE stable_vector::stable_vector Catch2::Catch2WithMain)
    endif()
endif()

if (BENCHMARK)
    find_package(benchmark)
    add_executable(benchmark_stable_vector benchmark.cpp)
    target_link_libraries(benchmark_stable_vector PRIVATE stable_vector::stable_vector benchmark::benchmark_main)
endif()


if (FUZZER)
    add_executable(fuzz_stable_vector fuzz.cpp)
    target_link_libraries(fuzz_stable_vector stable_vector::stable_vector)
    set_target_properties(fuzz_stable_vector PROPERTIES COMPILE_FLAGS "-fsanitize=address,undefined,fuzzer")
    set_target_properties(fuzz_stable_vector PROPERTIES LINK_FLAGS "-fsanitize=address,undefined,fuzzer")
endif()