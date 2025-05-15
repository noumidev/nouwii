# Require CMake 3.16
cmake_minimum_required(VERSION 3.16)

# Require C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(nouwii C)

# Set include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Set source files
set(SOURCES
    src/common/buffer.c
    src/common/file.c
    src/core/memory.c
    src/nouwii.c
)

# Set header files
set(HEADERS
    include/common/buffer.h
    include/common/config.h
    include/common/file.h
    include/common/types.h
    include/core/memory.h
    include/nouwii.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
