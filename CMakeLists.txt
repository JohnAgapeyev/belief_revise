cmake_minimum_required(VERSION 3.5.1)
project(belief_revise)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -pipe -march=native -fopenmp")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BELIEF_REV_SOURCES
    src/main.cpp
    src/file.cpp
    src/belief.cpp
    src/interactive.cpp
    src/utils.cpp
)

add_custom_target(minisat_all COMMAND make r
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/minisat_all
)

add_executable(belief_rev ${BELIEF_REV_SOURCES})

target_include_directories(belief_rev PRIVATE
    src
)

add_dependencies(belief_rev minisat_all)
