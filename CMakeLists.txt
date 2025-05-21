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
    src/common/bit.c
    src/common/bswap.c
    src/common/buffer.c
    src/common/file.c
    src/core/memory.c
    src/hw/broadway.c
    src/hw/dsp.c
    src/hw/hollywood.c
    src/hw/mi.c
    src/hw/pi.c
    src/nouwii.c
)

# Set header files
set(HEADERS
    include/common/bit.h
    include/common/bswap.h
    include/common/buffer.h
    include/common/config.h
    include/common/file.h
    include/common/types.h
    include/core/memory.h
    include/hw/broadway.h
    include/hw/dsp.h
    include/hw/hollywood.h
    include/hw/mi.h
    include/hw/pi.h
    include/nouwii.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
