cmake_minimum_required(VERSION 3.20)

project(incppbuild VERSION 0.1.0 LANGUAGES CXX)

option(INCPPBUILD_BUILD_TESTS "Build unit tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(incppbuild_lib
    src/ast/ASTParser.cpp
    src/cache/CacheManager.cpp
    src/core/change.cpp
    src/dependency/dependency_graph.cpp
    src/fingerprint/fingerprint.cpp
    src/normalization/Canonicalizer.cpp
)

target_include_directories(incppbuild_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(incppbuild_lib PUBLIC cxx_std_20)

add_executable(incppbuild apps/incppbuild/main.cpp)
target_link_libraries(incppbuild PRIVATE incppbuild_lib)

add_executable(incppbuild_demo apps/incppbuild/demo_main.cpp)
target_link_libraries(incppbuild_demo PRIVATE incppbuild_lib)

if(MSVC)
    target_compile_options(incppbuild_lib PRIVATE /W4 /permissive-)
else()
    target_compile_options(incppbuild_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(INCPPBUILD_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(incppbuild_tests tests/dependency_graph_test.cpp)
        target_link_libraries(incppbuild_tests PRIVATE incppbuild_lib)
        add_test(NAME dependency_graph COMMAND incppbuild_tests)
    endif()
endif()
