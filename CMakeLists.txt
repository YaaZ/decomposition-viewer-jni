cmake_minimum_required(VERSION 3.15)
project(decomposition_viewer_jni)

set(CMAKE_CXX_STANDARD 20)


# Compile GLSL
set(GLSL_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/glsl")

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${GLSL_SOURCE_DIRECTORY}/*.vert"
        "${GLSL_SOURCE_DIRECTORY}/*.tesc"
        "${GLSL_SOURCE_DIRECTORY}/*.tese"
        "${GLSL_SOURCE_DIRECTORY}/*.geom"
        "${GLSL_SOURCE_DIRECTORY}/*.frag"
        "${GLSL_SOURCE_DIRECTORY}/*.comp"
        )

foreach(GLSL ${GLSL_SOURCE_FILES})
    file(RELATIVE_PATH GLSL_RELATIVE "${GLSL_SOURCE_DIRECTORY}" "${GLSL}")

    set(SPIRV_RELATIVE_PATH "shaders/${GLSL_RELATIVE}.spv")
    set(SPIRV_PATH "${PROJECT_BINARY_DIR}/${SPIRV_RELATIVE_PATH}")
    get_filename_component(SPIRV_PATH_DIRECTORY ${SPIRV_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${SPIRV_PATH_DIRECTORY})

    set(CPP_SPIRV_RELATIVE_PATH "shaders-cpp/${GLSL_RELATIVE}.cpp")
    set(CPP_SPIRV_PATH "${PROJECT_BINARY_DIR}/${CPP_SPIRV_RELATIVE_PATH}")
    get_filename_component(CPP_SPIRV_PATH_DIRECTORY ${CPP_SPIRV_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${CPP_SPIRV_PATH_DIRECTORY})
    string(REGEX REPLACE "[-\\.]" "_" CPP_SPIRV_VARIABLE_NAME ${GLSL_RELATIVE})

    add_custom_command(
            OUTPUT ${SPIRV_PATH} ${CPP_SPIRV_PATH}
            COMMAND glslc ${GLSL} --target-env=vulkan1.1 -o ${SPIRV_PATH}
            COMMAND "${PROJECT_BINARY_DIR}/decomposition_viewer_jni_bin2cpp" ${SPIRV_PATH} ${CPP_SPIRV_PATH} "resource::shader" ${CPP_SPIRV_VARIABLE_NAME}
            DEPENDS ${GLSL}
            COMMENT "Compiling GLSL ${GLSL_RELATIVE}"
    )
    list(APPEND SPIRV_BINARY_FILES "${SPIRV_PATH}")
    list(APPEND CPP_SPIRV_BINARY_FILES "${CPP_SPIRV_PATH}")
endforeach()

add_custom_target(decomposition_viewer_jni_shaders DEPENDS ${SPIRV_BINARY_FILES})

# Compile bin2cpp
add_executable(decomposition_viewer_jni_bin2cpp bin2cpp.c)





# Compile C++
file(GLOB_RECURSE CPP_SOURCE_FILES "src/*.cpp")
add_library(decomposition_viewer_jni SHARED ${CPP_SOURCE_FILES} ${CPP_SPIRV_BINARY_FILES})


# Add dependencies
add_dependencies(decomposition_viewer_jni decomposition_viewer_jni_shaders)
add_dependencies(decomposition_viewer_jni_shaders decomposition_viewer_jni_bin2cpp)


# Find JNI
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# Find decomposition library
if(NOT DEFINED DECOMPOSITION_LIBRARY_PATH)
    set(DECOMPOSITION_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/lib/decomposition-library")
endif()
add_subdirectory(${DECOMPOSITION_LIBRARY_PATH} ${PROJECT_BINARY_DIR}/decomposition-library)
set_property(TARGET decomposition_library PROPERTY POSITION_INDEPENDENT_CODE ON)

# Link libraries
target_link_libraries(decomposition_viewer_jni decomposition_library ${JNI_LIBRARIES})

# Include VMA
include_directories(lib/VulkanMemoryAllocator/src)

# Include Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(decomposition_viewer_jni PUBLIC ${Vulkan_INCLUDE_DIRS})