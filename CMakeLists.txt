cmake_minimum_required(VERSION 3.16)
project(stable_vector)

set(CMAKE_CXX_STANDARD 20)

option(FUZZER OFF "Build a fuzz-tester")
option(BENCHMARK OFF "Build a benchmark")
option(UNIT_TEST OFF "Build unit test")

include(FetchContent)

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

if (UNIT_TEST)
    set(CMAKE_CXX_STANDARD_REQUIRED YES)
    set(CMAKE_CXX_EXTENSIONS OFF)

    find_package(Catch2 3 QUIET)
    if (NOT ${Catch2_FOUND})
        FetchContent_declare(
                Catch2
                GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                GIT_TAG ab6c7375be9a8e71ee84c6f8537113f9f47daf99 # 3.2.1
                OVERRIDE_FIND_PACKAGE
        )
        find_package(Catch2 3)
    endif()
    add_executable(test_stable_vector main.cpp)

    target_link_libraries(test_stable_vector PRIVATE stable_vector::stable_vector Catch2::Catch2WithMain)
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