cmake_minimum_required(VERSION 3.26)

project(witsh VERSION 0.1 DESCRIPTION "OS Module Shell Project Wits Shell" LANGUAGES C)

add_executable(witsh src/main.c src/util.c)

set(CMAKE_C_STANDARD 11)
set(C_STANDARD_REQUIRED 11)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_directories(${PROJECT_NAME}
    PRIVATE src
)
