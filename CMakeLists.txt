cmake_minimum_required(VERSION 3.20)
project(md1img_repacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
include(GNUInstallDirs)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS_RELEASE "-pipe -O2 -s")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)

include_directories(
        ${PROJECT_SOURCE_DIR}/src
)
set(ZLIB_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/adler32.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/compress.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/crc32.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/deflate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/gzclose.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/gzlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/gzread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/gzwrite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/infback.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/inffast.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/inflate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/inftrees.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/trees.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/uncompr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/zlib/zutil.c
)
add_library(z STATIC ${ZLIB_SRC_FILES})

file(GLOB SRCS
        "${PROJECT_SOURCE_DIR}/src/*"
)

add_executable(md1img_repacker ${SRCS})
target_link_libraries(md1img_repacker z)