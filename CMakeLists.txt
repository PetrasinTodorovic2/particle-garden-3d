cmake_minimum_required(VERSION 3.20)

project(ParticleGarden VERSION 4.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

add_executable(ParticleGarden
    src/main.cpp
    src/AppSettings.cpp
    src/Background.cpp
    src/ParticleSystem.cpp
)

target_include_directories(ParticleGarden PRIVATE src)
target_link_libraries(ParticleGarden PRIVATE sfml-graphics sfml-window sfml-system)

if(MSVC)
    target_compile_options(ParticleGarden PRIVATE /W4 /permissive- $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(ParticleGarden PRIVATE -Wall -Wextra -Wpedantic $<$<CONFIG:Release>:-O3>)
endif()

if(WIN32)
    target_link_libraries(ParticleGarden PRIVATE sfml-main)
    set_target_properties(ParticleGarden PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
