cmake_minimum_required(VERSION 3.18)

project(Trail3D LANGUAGES CXX)

set(TRAIL_ROOT ${CMAKE_CURRENT_LIST_DIR})

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v2.0.6
        SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(trail_glad STATIC REPRODUCIBLE LOADER API gl:core=3.3)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
)
FetchContent_MakeAvailable(assimp)

set(STB_DIR "${TRAIL_ROOT}/third_party")
set(STB_IMAGE_H "${STB_DIR}/stb_image.h")
if (NOT EXISTS "${STB_IMAGE_H}")
    file(MAKE_DIRECTORY "${STB_DIR}")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
            "${STB_IMAGE_H}"
            TLS_VERIFY ON
    )
endif ()

file(GLOB TRAIL_SOURCES CONFIGURE_DEPENDS
        "${TRAIL_ROOT}/src/*.h"
        "${TRAIL_ROOT}/src/*.cpp"
)

add_executable(Trail3D ${TRAIL_SOURCES})
target_link_libraries(Trail3D PRIVATE glfw trail_glad glm::glm assimp::assimp)

target_include_directories(Trail3D PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/gladsources/trail_glad/include"
        "${STB_DIR}"
)

if (WIN32)
    target_link_libraries(Trail3D PRIVATE opengl32)
endif ()
if (UNIX AND NOT APPLE)
    target_link_libraries(Trail3D PRIVATE dl pthread)
endif ()

if (MSVC)
    target_compile_options(Trail3D PRIVATE /std:c++17 /MP)
    target_link_options(Trail3D PUBLIC /ignore:4099)
else ()
    target_compile_features(Trail3D PRIVATE cxx_std_17)
endif ()

set_target_properties(Trail3D PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TRAIL_ROOT}/bin/Trail3D"
)

if (NOT APPLE)
    add_custom_command(TARGET Trail3D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TRAIL_ROOT}/assets"
            "$<TARGET_FILE_DIR:Trail3D>/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TRAIL_ROOT}/resource"
            "$<TARGET_FILE_DIR:Trail3D>/resource"
            VERBATIM
    )
endif ()
