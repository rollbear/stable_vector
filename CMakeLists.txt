cmake_minimum_required(VERSION 3.16)
project(stable_vector)

set(CMAKE_CXX_STANDARD 20)

add_library(stable_vector INTERFACE)
add_library(stable_vector::stable_vector ALIAS stable_vector)
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(stable_vector INTERFACE $<BUILD_INTERFACE:${INCLUDE_DIR}>)
target_include_directories(stable_vector INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
add_executable(test_stable_vector main.cpp )
if (CATCH2_PREFIX)
    target_link_directories(test_stable_vector PRIVATE ${CATCH2_PREFIX}/lib)
    target_include_directories(test_stable_vector PRIVATE ${CATCH2_PREFIX}/include)
endif()

target_link_libraries(test_stable_vector PRIVATE stable_vector::stable_vector Catch2Main Catch2)

install(TARGETS stable_vector EXPORT stable_vector-targets INCLUDES DESTINATION include)
install(EXPORT stable_vector-targets DESTINATION lib/cmake/stable_vector)
install(FILES stable_vector-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/stable_vector/stable_vector-config-version.cmake
        DESTINATION lib/cmake/stable_vector COMPONENT Devel)
install(DIRECTORY "include" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
