cmake_minimum_required(VERSION 3.5.1)
project(belief_revise)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -pipe -march=native -fopenmp")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BELIEF_REV_SOURCES
    src/main.cpp
    src/file.cpp
    src/belief.cpp
)

add_executable(belief_rev ${BELIEF_REV_SOURCES})

find_package(OpenSSL)

target_link_libraries(belief_rev OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(belief_rev PRIVATE
    src
)
