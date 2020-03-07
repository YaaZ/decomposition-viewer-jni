cmake_minimum_required(VERSION 3.15)
project(decomposition_viewer_jni)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE CPP_SOURCE_FILES "src/*.cpp")
add_library(decomposition_viewer_jni SHARED ${CPP_SOURCE_FILES})

find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})



if(NOT DEFINED DECOMPOSITION_LIBRARY_PATH)
    set(DECOMPOSITION_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/lib/decomposition-library")
endif()


add_subdirectory(${DECOMPOSITION_LIBRARY_PATH} ${PROJECT_BINARY_DIR}/decomposition-library)

set_property(TARGET decomposition_library PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(decomposition_viewer_jni decomposition_library)