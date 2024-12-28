cmake_minimum_required(VERSION 3.30)
project(scroller C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_C_STANDARD 90)

add_executable(${CMAKE_PROJECT_NAME}
        src/main.c
        src/mem.c
        src/graphics.c
)
