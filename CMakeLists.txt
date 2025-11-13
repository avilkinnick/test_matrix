cmake_minimum_required(VERSION 3.10)

project(test_matrix)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(TARGET_NAME test_matrix)

add_executable(${TARGET_NAME} ${PROJECT_SOURCE_DIR}/main.c)

target_link_libraries(${TARGET_NAME}
    m
    OpenGL::GL
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_ttf::SDL2_ttf
)

add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
)

if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W3)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
