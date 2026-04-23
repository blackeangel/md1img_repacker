cmake_minimum_required(VERSION 3.20...4.9)
project(md1img_repacker LANGUAGES C CXX)

# ---- стандарты ----
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- тип сборки ----
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s")

# =========================================================
# ===================== ЛИНКОВКА ==========================
# =========================================================
#
# Три сценария:
#
#   1. NDK кросс-компиляция (CMAKE_CROSSCOMPILING=ON, CMAKE_ANDROID_NDK задан)
#      → полный -static, STL через ANDROID_STL=c++_static в команде cmake
#
#   2. Нативно в Termux (ANDROID=1, но NDK не используется)
#      → только -static-libstdc++, так как bionic libc.so не имеет
#        статического варианта по дизайну Android
#
#   3. Linux / Windows / macOS
#      → полный -static как в оригинале
#
if(CMAKE_CROSSCOMPILING AND CMAKE_ANDROID_NDK)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    message(STATUS "Build mode: NDK cross-compile → full static")
elseif(ANDROID)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
    message(STATUS "Build mode: native Termux → static-libstdc++ only")
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    message(STATUS "Build mode: desktop → full static")
endif()

# =========================================================
# ===================== THREADS ============================
# =========================================================
#
# NDK: find_package(Threads) работает штатно.
# Termux нативно: pthread встроен в bionic — удовлетворяем CMake вручную.
#
if(ANDROID AND NOT CMAKE_CROSSCOMPILING)
    set(CMAKE_THREAD_LIBS_INIT     ""   CACHE STRING "" FORCE)
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(Threads_FOUND              TRUE CACHE BOOL   "" FORCE)
    if(NOT TARGET Threads::Threads)
        add_library(Threads::Threads INTERFACE IMPORTED GLOBAL)
    endif()
else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

include(FetchContent)

# =========================================================
# ===================== zlib ==============================
# =========================================================
#
# Официальный репозиторий zlib от автора (Mark Adler):
# https://github.com/madler/zlib
# Содержит полноценный CMakeLists.txt, собирается точно так же как xz.
# После этого src/zlib/ можно удалить из репозитория.
#
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL  # CMake 3.28+: таргеты подпроекта не входят в ALL
)

# Отключаем примеры и тесты — нужна только статическая zlibstatic.
# ZLIB_BUILD_EXAMPLES выключает примеры, но не тесты (minigzip, zlib_example).
# EXCLUDE_FROM_ALL в FetchContent_Declare (CMake 3.28+) — эквивалент
# add_subdirectory(EXCLUDE_FROM_ALL): таргеты подпроекта не включаются
# в цель ALL, поэтому тесты не собираются и не пытаются линковать libz.so.
# zlibstatic всё равно соберётся как зависимость нашего таргета.
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zlib)

# madler/zlib создаёт два таргета: zlib (shared) и zlibstatic (static).
# Используем zlibstatic. zlib.h будет в build/_deps/zlib-src/ и zlib-build/.
# Прописываем include-пути на нашем таргете (аналогично lzma.h).

# =========================================================
# ===================== XZ / liblzma =======================
# =========================================================

FetchContent_Declare(
    xz
    GIT_REPOSITORY https://github.com/tukaani-project/xz.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

set(BUILD_SHARED_LIBS    OFF CACHE BOOL "" FORCE)
set(XZ_BUILD_ENCODERS    ON  CACHE BOOL "" FORCE)
set(XZ_BUILD_DECODERS    ON  CACHE BOOL "" FORCE)
# Threads нужны только для параллельного сжатия — для прошивок не нужно.
set(XZ_ENABLE_THREADS    OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZ           OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZDEC        OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_LZMADEC      OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_LZMAINFO     OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZ_GREP      OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZ_LESS      OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZ_DIFF      OFF CACHE BOOL "" FORCE)
set(CREATE_XZ_SYMLINKS   OFF CACHE BOOL "" FORCE)
set(CREATE_LZMA_SYMLINKS OFF CACHE BOOL "" FORCE)
set(XZ_ENABLE_DOXYGEN    OFF CACHE BOOL "" FORCE)
set(XZ_ENABLE_SANDBOX    OFF CACHE BOOL "" FORCE)
set(ENABLE_NLS           OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(xz)

# =========================================================
# ===================== EXECUTABLE =========================
# =========================================================

include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB SRCS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.h
)

add_executable(md1img_repacker ${SRCS})

# zlib.h и lzma.h лежат внутри скачанных FetchContent исходников.
# Прописываем пути на нашем таргете — не вызывает ошибок CMake 4.x.
target_include_directories(md1img_repacker PRIVATE
    ${zlib_SOURCE_DIR}          # zlib.h (исходники)
    ${zlib_BINARY_DIR}          # zconf.h (генерируется cmake zlib в build dir)
    ${xz_SOURCE_DIR}/src/liblzma/api
)

target_link_libraries(md1img_repacker
    zlibstatic
    liblzma
    Threads::Threads
)

if(WIN32)
    target_compile_definitions(md1img_repacker PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# =========================================================
set_target_properties(md1img_repacker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out
)
