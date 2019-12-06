cmake_minimum_required(VERSION 3.12)
project(kipper VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory(src)
add_subdirectory(apps)

if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME OR KIPPER_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()