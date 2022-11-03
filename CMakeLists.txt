cmake_minimum_required(VERSION 3.14)

project(NativeLINQ)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ./Src)

target_sources(${PROJECT_NAME}
INTERFACE
    Src/NativeLINQ/NativeLINQ.h
)