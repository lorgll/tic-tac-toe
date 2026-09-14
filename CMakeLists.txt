cmake_minimum_required(VERSION 3.10)
project(tic_tac_toe LANGUAGES C VERSION 0.2.0-alpha.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_executable(${PROJECT_NAME})
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()
target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(${PROJECT_NAME}
    PRIVATE
      src/main.c
      src/terminal.c
      src/renderer.c
      src/game.c
)
