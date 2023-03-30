cmake_minimum_required (VERSION 3.8)

project(BN3MONKEY_LIBRARY)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message("SOURCE_DIR is ${SOURCE_DIR}")

file(GLOB_RECURSE SOURCE_SUB_DIR
    )

file(GLOB_RECURSE SOURCE_FILES
    ${SOURCE_DIR}/*.h
    ${SOURCE_DIR}/*.hpp
    ${SOURCE_DIR}/*.cpp
    ${SOURCE_DIR}/*.c)
message("SOURCE_FILES is ${SOURCE_FILES}")


add_library(bn3monkey_library
    STATIC
    ${SOURCE_FILES})
    
target_compile_definitions(bn3monkey_library PUBLIC BN3MONKEY_DEBUG)

set_property(TARGET bn3monkey_library PROPERTY CXX_STANDARD 17)
set_property(TARGET bn3monkey_library PROPERTY CXX_STANDARD_REQUIRED ON)