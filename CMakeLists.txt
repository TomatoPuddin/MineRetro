cmake_minimum_required(VERSION 3.8)
project(mineretro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(MR_WINDOWS _AMD64_
            _CRT_SECURE_NO_WARNINGS _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
            UNICODE _UNICODE
            WINVER=0x0601 _WIN32_WINNT=0x0601)
    add_compile_options(/MT$<$<CONFIG:Debug>:d> /utf-8 /permissive- /volatile:iso)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/EHsc -flto /Gw)
        add_link_options(-flto /OPT:REF /OPT:ICF)
    else()
        add_compile_options(/EHa)
    endif()
else()
    add_compile_definitions(MR_UNIXLIKE)

    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_SKIP_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

    add_compile_options(-pthread)
    if(LINUX)
        add_compile_definitions(MR_LINUX)
        add_compile_options(-Wno-deprecated-declarations)
        add_link_options(-static-libstdc++ -ldl -pthread -Wl,--no-undefined)
        if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
            add_link_options(-static-libgcc)
        endif()

        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_compile_options(-flto -fdata-sections -ffunction-sections -fno-addrsig -fomit-frame-pointer)
            add_link_options(-s -flto -Wl,--lto-whole-program-visibility
                    -Wl,--icf=all -Wl,--no-rosegment -Wl,--gc-sections
                    -Wl,--version-script -Wl,${PROJECT_SOURCE_DIR}/version-script)
        endif()
    elseif(APPLE)
        add_compile_definitions(MR_MACOS)
    else()
        message(FATAL_ERROR "Unsupported OS")
    endif()
endif()

add_library(Mineretro SHARED src/mineretro.cc src/platforms/impl.cc)
