cmake_minimum_required(VERSION 3.12)

project(sgc)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(SOURCES
        src/sgc.c)

add_library(sgc_lib
        STATIC
        ${SOURCES})

target_include_directories(sgc_lib
        PRIVATE
        include)
