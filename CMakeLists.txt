cmake_minimum_required(VERSION 3.24)
project(NostaleXigncodeDisabler VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This project requires Windows and the Microsoft C++ toolchain.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
option(XC3_BUILD_TESTS "Build focused launcher safety checks" OFF)
include(cmake/client-profiles.cmake)

add_library(local_launcher STATIC
    src/launcher/adjacent_client_launcher.cpp
    src/launcher/launch_options.cpp
    src/launcher/local_client_launcher.cpp
    src/launcher/xigncode_startup.cpp
    src/platform/sha256.cpp)
target_include_directories(local_launcher PUBLIC include "${profile_include_directory}")
target_compile_definitions(local_launcher PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(local_launcher PRIVATE /W4 /WX /permissive- /utf-8 /sdl /guard:cf)
target_link_libraries(local_launcher PRIVATE advapi32 bcrypt shell32 user32)

add_executable(xigncode_disabler src/main.cpp)
target_link_libraries(xigncode_disabler PRIVATE local_launcher)
target_compile_options(xigncode_disabler PRIVATE /W4 /WX /permissive- /utf-8 /sdl /guard:cf)
target_link_options(xigncode_disabler PRIVATE /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf /INCREMENTAL:NO)
set_target_properties(xigncode_disabler PROPERTIES OUTPUT_NAME "Nostale-Xigncode-Disabler")

if(XC3_BUILD_TESTS)
    enable_testing()
    add_executable(launch_options_tests tests/launch_options_tests.cpp src/launcher/launch_options.cpp)
    target_compile_options(launch_options_tests PRIVATE /W4 /WX /permissive- /utf-8)
    add_test(NAME launch_options_and_loopback_guards COMMAND launch_options_tests)
endif()
