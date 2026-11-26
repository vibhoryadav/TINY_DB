cmake_minimum_required(VERSION 3.20)

project(
  tinydb
  VERSION 0.1.0
  DESCRIPTION "Educational disk-oriented relational database management system in C++20"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TINYDB_BUILD_TESTS "Build TinyDB automated test suites" ON)
option(TINYDB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(TINYDB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(tinydb_compile_options INTERFACE)

if(MSVC)
  target_compile_options(tinydb_compile_options INTERFACE /W4)
  if(TINYDB_WARNINGS_AS_ERRORS)
    target_compile_options(tinydb_compile_options INTERFACE /WX)
  endif()
else()
  target_compile_options(
    tinydb_compile_options
    INTERFACE -Wall
              -Wextra
              -Wpedantic
              -Wshadow
              -Wconversion
              -Wsign-conversion)
  if(TINYDB_WARNINGS_AS_ERRORS)
    target_compile_options(tinydb_compile_options INTERFACE -Werror)
  endif()
  if(TINYDB_ENABLE_SANITIZERS)
    target_compile_options(
      tinydb_compile_options
      INTERFACE -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -g)
    target_link_options(
      tinydb_compile_options
      INTERFACE -fsanitize=address,undefined)
  endif()
endif()

add_library(tinydb_core STATIC
  src/core/build_info.cpp
)

target_include_directories(
  tinydb_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(tinydb_core PUBLIC tinydb_compile_options)

add_executable(tinydb
  src/cli/main.cpp
)

target_link_libraries(tinydb PRIVATE tinydb_core)

if(TINYDB_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  include(GoogleTest)

  add_executable(tinydb_unit_tests
    tests/unit/smoke_test.cpp
  )
  target_link_libraries(tinydb_unit_tests PRIVATE tinydb_core GTest::gtest_main)
  gtest_discover_tests(tinydb_unit_tests)
endif()
