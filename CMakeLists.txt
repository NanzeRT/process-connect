cmake_minimum_required(VERSION 3.27)

project(ProjectName LANGUAGES C)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS
    src/*.c
    include/*.h
)

add_executable(process-connect ${sources})
target_include_directories(process-connect PRIVATE src include)
