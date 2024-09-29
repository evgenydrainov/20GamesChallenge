cmake_minimum_required(VERSION 3.20)
project(04Asteroids-2)

# 
# For Linux.
# 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
add_compile_definitions($<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

include_directories(${PROJECT_NAME} ../external/glad/include)
include_directories(${PROJECT_NAME} ../external/glm/include)
include_directories(${PROJECT_NAME} ../external/stb/include)

if(NOT SDL2_INCLUDE_DIRS)
	find_package(SDL2 REQUIRED)
endif()

MESSAGE(STATUS "INFO: SDL2_INCLUDE_DIRS is ${SDL2_INCLUDE_DIRS}")
MESSAGE(STATUS "INFO: SDL2_LIBRARIES is ${SDL2_LIBRARIES}")

include_directories(${PROJECT_NAME} ${SDL2_INCLUDE_DIRS})

set(SOURCES
	src/batch_renderer.cpp
	src/font.cpp
	src/game.cpp
	src/main.cpp
	src/package.cpp
	src/texture.cpp
	src/window_creation.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${PROJECT_NAME} SDL2_mixer)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/stdafx.h)
