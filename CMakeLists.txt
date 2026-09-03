cmake_minimum_required(VERSION 3.20)

project(incppbuild VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(INCPPBUILD_BUILD_TESTS "Build unit tests" ON)
option(INCPPBUILD_ENABLE_CLANG_TOOLING "Build the Clang LibTooling parser" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(incppbuild_lib
    src/ast/ASTParser.cpp
    src/build/CompilerDriver.cpp
    src/build/IncrementalBuilder.cpp
    src/cache/CacheManager.cpp
    src/core/CompilationDatabase.cpp
    src/core/change.cpp
    src/dependency/dependency_graph.cpp
    src/fingerprint/fingerprint.cpp
    src/normalization/Canonicalizer.cpp
)

if(INCPPBUILD_ENABLE_CLANG_TOOLING)
    find_package(LLVM CONFIG REQUIRED)
    find_package(Clang CONFIG REQUIRED)

    add_library(incppbuild_clang_ast
        src/ast/ClangASTParser.cpp
        src/dependency/DependencyAnalyzer.cpp
    )
    target_link_libraries(incppbuild_clang_ast
        PUBLIC incppbuild_lib
        PRIVATE clangTooling clangFrontend clangAST clangBasic clangLex
    )
    target_include_directories(incppbuild_clang_ast SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(incppbuild_clang_ast PRIVATE ${LLVM_DEFINITIONS})
endif()

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

        add_executable(incppbuild_compilation_database_tests tests/compilation_database_test.cpp)
        target_link_libraries(incppbuild_compilation_database_tests PRIVATE incppbuild_lib)
        add_test(NAME compilation_database COMMAND incppbuild_compilation_database_tests)

        add_executable(incppbuild_fingerprint_tests tests/fingerprint_test.cpp)
        target_link_libraries(incppbuild_fingerprint_tests PRIVATE incppbuild_lib)
        add_test(NAME fingerprint COMMAND incppbuild_fingerprint_tests)

        add_executable(incppbuild_cache_tests tests/cache_manager_test.cpp)
        target_link_libraries(incppbuild_cache_tests PRIVATE incppbuild_lib)
        add_test(NAME cache_manager COMMAND incppbuild_cache_tests)

        add_test(NAME correctness
            COMMAND ${CMAKE_COMMAND}
                -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/correctness_test.cmake)
    endif()
endif()
