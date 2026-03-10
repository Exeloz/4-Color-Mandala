cmake_minimum_required(VERSION 3.22.1)
project(color_mandala_android LANGUAGES C CXX)

set(APP_LIB_NAME "colormandala" CACHE STRING "Native library name")

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../")
set(APP_SRC_DIR "${PROJECT_ROOT}/app/src")
set(LIBTESS2_DIR "${PROJECT_ROOT}/external/libtess2/Source")
set(RAYLIB_ROOT_DEFAULT "${PROJECT_ROOT}/external/raylib")
if(DEFINED ENV{RAYLIB_ROOT} AND NOT "$ENV{RAYLIB_ROOT}" STREQUAL "")
    set(RAYLIB_ROOT_DEFAULT "$ENV{RAYLIB_ROOT}")
endif()
set(RAYLIB_ROOT "${RAYLIB_ROOT_DEFAULT}" CACHE PATH "Path to raylib source root")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${APP_SRC_DIR}/*.cpp"
)

file(GLOB LIBTESS2_SOURCES CONFIGURE_DEPENDS
    "${LIBTESS2_DIR}/*.c"
)

if(NOT EXISTS "${RAYLIB_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "raylib source not found at: ${RAYLIB_ROOT}")
endif()

if(NOT EXISTS "${RAYLIB_ROOT}/src/raylib.h")
    message(FATAL_ERROR "raylib.h not found at: ${RAYLIB_ROOT}/src/raylib.h")
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${RAYLIB_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/raylib-build")

add_library(${APP_LIB_NAME} SHARED
    ${APP_SOURCES}
    ${LIBTESS2_SOURCES}
)

target_include_directories(${APP_LIB_NAME} PRIVATE
    "${APP_SRC_DIR}"
    "${LIBTESS2_DIR}/../Include"
    "${RAYLIB_ROOT}/src"
    "${RAYLIB_ROOT}/src/external"
)

target_compile_definitions(${APP_LIB_NAME} PRIVATE
    PLATFORM_ANDROID
    ANDROID
)

target_link_options(${APP_LIB_NAME} PRIVATE
    -u ANativeActivity_onCreate
)

target_link_libraries(${APP_LIB_NAME}
    raylib
    android
    log
    EGL
    GLESv2
    OpenSLES
    dl
    m
)
