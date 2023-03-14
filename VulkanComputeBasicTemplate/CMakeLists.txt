cmake_minimum_required(VERSION 3.25)
project(VulkanComputeBasicTemplate)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES Source/*.cpp Source/*.h)

add_executable(VulkanComputeBasicTemplate ${SOURCES})

find_package(Vulkan REQUIRED SPIRV-Tools glslang shaderc_combined)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})

if((${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND WIN32)
    target_link_libraries(${PROJECT_NAME}
            ${Vulkan_LIBRARIES}
            ${Vulkan_SPIRV-Tools_DEBUG_LIBRARY}
            ${Vulkan_glslang_DEBUG_LIBRARY}
            ${Vulkan_shaderc_combined_DEBUG_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME}
            ${Vulkan_LIBRARIES}
            ${Vulkan_SPIRV-Tools_LIBRARY}
            ${Vulkan_glslang_LIBRARY}
            ${Vulkan_shaderc_combined_LIBRARY})
endif()