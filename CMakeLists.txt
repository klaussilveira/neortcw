cmake_minimum_required(VERSION 3.12)
project(iortcw VERSION 1.51 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Top-level build options
option(BUILD_MP "Build multiplayer targets" ON)
option(BUILD_SP "Build singleplayer targets" ON)
option(BUILD_ET "Build Enemy Territory targets" ON)

option(USE_RENDERER_DLOPEN "Load renderer as shared library" ON)
option(USE_OPENAL "Use OpenAL for sound" ON)
option(USE_OPENAL_DLOPEN "dlopen OpenAL at runtime" ON)
option(USE_CODEC_VORBIS "Enable Ogg Vorbis codec" ON)
option(USE_CODEC_OPUS "Enable Opus codec" ON)
option(USE_VOIP "Enable VoIP support" ON)
option(USE_MUMBLE "Enable Mumble link support" ON)
option(USE_FREETYPE "Enable FreeType font rendering" ON)
option(USE_BLOOM "Enable bloom rendering effect" ON)

option(USE_INTERNAL_LIBS "Use all bundled libraries" OFF)
option(USE_INTERNAL_ZLIB "Use bundled zlib" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_JPEG "Use bundled libjpeg" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_FREETYPE "Use bundled FreeType" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_OGG "Use bundled libogg" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_VORBIS "Use bundled libvorbis" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_OPUS "Use bundled Opus" ${USE_INTERNAL_LIBS})

# Platform detection
if(EMSCRIPTEN)
    set(PLATFORM "emscripten")

    # Disable features unavailable in browser
    set(USE_RENDERER_DLOPEN OFF CACHE BOOL "" FORCE)
    set(USE_OPENAL OFF CACHE BOOL "" FORCE)
    set(USE_OPENAL_DLOPEN OFF CACHE BOOL "" FORCE)
    set(USE_VOIP OFF CACHE BOOL "" FORCE)
    set(USE_MUMBLE OFF CACHE BOOL "" FORCE)
    set(USE_FREETYPE OFF CACHE BOOL "" FORCE)
    set(USE_CODEC_VORBIS OFF CACHE BOOL "" FORCE)
    set(USE_CODEC_OPUS OFF CACHE BOOL "" FORCE)
    set(USE_INTERNAL_LIBS ON CACHE BOOL "" FORCE)
    set(USE_INTERNAL_ZLIB ON CACHE BOOL "" FORCE)
    set(USE_INTERNAL_JPEG ON CACHE BOOL "" FORCE)
    set(BUILD_SP OFF CACHE BOOL "" FORCE)
    set(BUILD_MP OFF CACHE BOOL "" FORCE)
    set(HAVE_VM_COMPILED FALSE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM "darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM "freebsd")
elseif(WIN32)
    set(PLATFORM "mingw32")
else()
    set(PLATFORM "${CMAKE_SYSTEM_NAME}")
endif()

# Architecture
if(EMSCRIPTEN)
    set(ARCH "wasm32")
    set(FILE_ARCH "wasm32")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    set(ARCH "x86_64")
    set(FILE_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86)$")
    set(ARCH "x86")
    if(WIN32)
        set(FILE_ARCH "x86")
    else()
        set(FILE_ARCH "i386")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(ARCH "arm64")
    set(FILE_ARCH "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7")
    set(ARCH "armv7l")
    set(FILE_ARCH "armv7l")
else()
    set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    set(FILE_ARCH "${ARCH}")
endif()

# Shared library extension
if(EMSCRIPTEN)
    set(SHLIBEXT "wasm")
elseif(APPLE)
    set(SHLIBEXT "dylib")
elseif(WIN32)
    set(SHLIBEXT "dll")
else()
    set(SHLIBEXT "so")
endif()

set(SHLIBNAME "${FILE_ARCH}.${SHLIBEXT}")

# Game module naming: Windows uses underscores, Unix uses dots
if(WIN32)
    set(MP_GAMEMOD_SHLIBNAME "_mp_${SHLIBNAME}")
    set(SP_GAMEMOD_SHLIBNAME "_sp_${SHLIBNAME}")
    set(ET_GAMEMOD_SHLIBNAME "_et_${SHLIBNAME}")
else()
    set(MP_GAMEMOD_SHLIBNAME ".mp.${SHLIBNAME}")
    set(SP_GAMEMOD_SHLIBNAME ".sp.${SHLIBNAME}")
    set(ET_GAMEMOD_SHLIBNAME ".et.${SHLIBNAME}")
endif()

message(STATUS "Platform: ${PLATFORM}")
message(STATUS "Architecture: ${ARCH}")
message(STATUS "Shared library suffix: ${SHLIBNAME}")

# Git version string (base, subdirs append SP/MP suffix)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} show -s --pretty=format:%h-%ad --date=short
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REV
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# Compiler flags
set(HAVE_VM_COMPILED FALSE)
if(ARCH MATCHES "^(x86|x86_64|armv7l|ppc|sparc)$")
    set(HAVE_VM_COMPILED TRUE)
endif()

# Platform-specific shared library flags
if(PLATFORM STREQUAL "linux" OR PLATFORM STREQUAL "freebsd")
    set(SHLIBCFLAGS -fPIC -fvisibility=hidden)
elseif(PLATFORM STREQUAL "darwin")
    set(SHLIBCFLAGS -fPIC -fno-common)
elseif(WIN32)
    set(SHLIBCFLAGS "")
endif()

# Static MinGW runtime for distributable Windows builds
if(WIN32 AND MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")
endif()

# Find external libraries

# SDL2
if((BUILD_MP OR BUILD_SP OR BUILD_ET) AND NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif()

# ZLIB
if(NOT USE_INTERNAL_ZLIB)
    find_package(ZLIB)
    if(NOT ZLIB_FOUND)
        message(STATUS "System zlib not found, using internal")
        set(USE_INTERNAL_ZLIB ON CACHE BOOL "" FORCE)
    endif()
endif()

# JPEG
if(NOT USE_INTERNAL_JPEG)
    find_package(JPEG)
    if(NOT JPEG_FOUND)
        message(STATUS "System libjpeg not found, using internal")
        set(USE_INTERNAL_JPEG ON CACHE BOOL "" FORCE)
    endif()
endif()

# FreeType
if(USE_FREETYPE)
    if(NOT USE_INTERNAL_FREETYPE)
        find_package(Freetype)
        if(NOT FREETYPE_FOUND)
            message(STATUS "System FreeType not found, using internal")
            set(USE_INTERNAL_FREETYPE ON CACHE BOOL "" FORCE)
        endif()
    endif()
endif()

# OpenAL
if(USE_OPENAL AND NOT USE_OPENAL_DLOPEN)
    find_package(OpenAL)
endif()

# Ogg/Vorbis/Opus
include(FindPkgConfig OPTIONAL)

set(NEED_OPUS OFF)
set(NEED_OGG OFF)

if(USE_VOIP OR USE_CODEC_OPUS)
    set(NEED_OPUS ON)
    set(NEED_OGG ON)
endif()
if(USE_CODEC_VORBIS)
    set(NEED_OGG ON)
endif()

if(NEED_OPUS AND NOT USE_INTERNAL_OPUS)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(OPUS opusfile opus)
    endif()
    if(NOT OPUS_FOUND)
        message(STATUS "System opus not found, using internal")
        set(USE_INTERNAL_OPUS ON CACHE BOOL "" FORCE)
    else()
        # opus.pc reports -I.../include/opus but code uses <opus/opus.h>
        # Add parent directories so the subdirectory-style includes work
        set(_opus_extra_dirs)
        foreach(dir ${OPUS_INCLUDE_DIRS})
            get_filename_component(parent "${dir}" DIRECTORY)
            list(APPEND _opus_extra_dirs "${parent}")
        endforeach()
        list(APPEND OPUS_INCLUDE_DIRS ${_opus_extra_dirs})
        list(REMOVE_DUPLICATES OPUS_INCLUDE_DIRS)
    endif()
endif()

if(USE_CODEC_VORBIS AND NOT USE_INTERNAL_VORBIS)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(VORBIS vorbisfile vorbis)
    endif()
    if(NOT VORBIS_FOUND)
        message(STATUS "System vorbis not found, using internal")
        set(USE_INTERNAL_VORBIS ON CACHE BOOL "" FORCE)
    endif()
endif()

if(NEED_OGG AND NOT USE_INTERNAL_OGG)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(OGG ogg)
    endif()
    if(NOT OGG_FOUND)
        message(STATUS "System ogg not found, using internal")
        set(USE_INTERNAL_OGG ON CACHE BOOL "" FORCE)
    endif()
endif()

# pkg-config library dirs (needed on macOS where Homebrew paths are non-default)
if(OPUS_LIBRARY_DIRS)
    link_directories(${OPUS_LIBRARY_DIRS})
endif()
if(VORBIS_LIBRARY_DIRS)
    link_directories(${VORBIS_LIBRARY_DIRS})
endif()
if(OGG_LIBRARY_DIRS)
    link_directories(${OGG_LIBRARY_DIRS})
endif()

# Internal libraries (built once, shared by SP and MP)
# NOTE: must come after detection above so USE_INTERNAL_* flags are final
add_subdirectory(libs)

# Helper functions (used by both MP/ and SP/ CMakeLists.txt)

# Common compile definitions based on feature options
function(add_wolf_definitions target)
    if(USE_OPENAL)
        target_compile_definitions(${target} PRIVATE USE_OPENAL)
        if(USE_OPENAL_DLOPEN)
            target_compile_definitions(${target} PRIVATE USE_OPENAL_DLOPEN)
        endif()
    endif()

    if(USE_CODEC_VORBIS)
        target_compile_definitions(${target} PRIVATE USE_CODEC_VORBIS)
    endif()

    if(USE_CODEC_OPUS)
        target_compile_definitions(${target} PRIVATE USE_CODEC_OPUS)
    endif()

    if(USE_VOIP)
        target_compile_definitions(${target} PRIVATE USE_VOIP)
    endif()

    if(USE_MUMBLE)
        target_compile_definitions(${target} PRIVATE USE_MUMBLE)
    endif()

    if(USE_RENDERER_DLOPEN)
        target_compile_definitions(${target} PRIVATE USE_RENDERER_DLOPEN)
    endif()

    if(USE_BLOOM)
        target_compile_definitions(${target} PRIVATE USE_BLOOM)
    endif()

    if(USE_INTERNAL_JPEG)
        target_compile_definitions(${target} PRIVATE USE_INTERNAL_JPEG)
    endif()

    if(USE_FREETYPE)
        target_compile_definitions(${target} PRIVATE BUILD_FREETYPE)
    endif()
endfunction()

# Link internal/external zlib
function(link_wolf_libs target)
    if(USE_INTERNAL_ZLIB)
        target_link_libraries(${target} PRIVATE internal_zlib)
    else()
        target_link_libraries(${target} PRIVATE ZLIB::ZLIB)
    endif()
endfunction()

# Link renderer dependencies (SDL2, JPEG, FreeType, OpenGL)
function(link_renderer_libs target)
    if(NOT EMSCRIPTEN)
        target_include_directories(${target} PRIVATE ${SDL2_INCLUDE_DIR})
        target_link_libraries(${target} PRIVATE ${SDL2_LIBRARY})
    endif()

    if(USE_INTERNAL_JPEG)
        target_link_libraries(${target} PRIVATE internal_jpeg)
    else()
        target_link_libraries(${target} PRIVATE ${JPEG_LIBRARIES})
        target_include_directories(${target} PRIVATE ${JPEG_INCLUDE_DIRS})
    endif()

    if(USE_FREETYPE)
        if(USE_INTERNAL_FREETYPE)
            target_link_libraries(${target} PRIVATE internal_freetype)
        else()
            target_link_libraries(${target} PRIVATE ${FREETYPE_LIBRARIES})
            target_include_directories(
                ${target}
                PRIVATE ${FREETYPE_INCLUDE_DIRS}
            )
        endif()
    endif()

    if(NOT EMSCRIPTEN)
        find_package(OpenGL REQUIRED)
        target_link_libraries(${target} PRIVATE OpenGL::GL)
    endif()
endfunction()

# Link codec libraries (Opus, Vorbis, Ogg)
function(link_client_codec_libs target)
    if(NEED_OPUS)
        if(USE_INTERNAL_OPUS)
            target_link_libraries(
                ${target}
                PRIVATE internal_opus internal_opusfile
            )
        else()
            target_link_libraries(${target} PRIVATE ${OPUS_LIBRARIES})
            target_include_directories(${target} PRIVATE ${OPUS_INCLUDE_DIRS})
        endif()
    endif()

    if(USE_CODEC_VORBIS)
        if(USE_INTERNAL_VORBIS)
            target_link_libraries(${target} PRIVATE internal_vorbis)
        else()
            target_link_libraries(${target} PRIVATE ${VORBIS_LIBRARIES})
            target_include_directories(${target} PRIVATE ${VORBIS_INCLUDE_DIRS})
        endif()
    endif()

    if(NEED_OGG)
        if(USE_INTERNAL_OGG)
            target_link_libraries(${target} PRIVATE internal_ogg)
        else()
            target_link_libraries(${target} PRIVATE ${OGG_LIBRARIES})
            target_include_directories(${target} PRIVATE ${OGG_INCLUDE_DIRS})
        endif()
    endif()
endfunction()

# Subdirectories
if(BUILD_MP)
    add_subdirectory(MP)
endif()

if(BUILD_SP)
    add_subdirectory(SP)
endif()

if(BUILD_ET)
    add_subdirectory(ET)
endif()
