cmake_minimum_required(VERSION 3.20)
project(vulkan)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD, "20")
set(CMAKE_CXX_STANDARD_REQUIRED, ON)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

find_package(Vulkan REQUIRED)
if(NOT Vulkan_glslc_FOUND)
    message(FATAL_ERROR "glslc is required to be installed!")
endif()

file(GLOB vulkan-renderer-srcs CONFIGURE_DEPENDS
    "./src/*.cpp"
)

add_executable(vulkan-renderer ${vulkan-renderer-srcs})

add_subdirectory(third-party)

target_link_libraries(vulkan-renderer Vulkan::Vulkan)
target_link_libraries(vulkan-renderer glfw)
target_link_libraries(vulkan-renderer spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

target_precompile_headers(vulkan-renderer
    PUBLIC
    <vulkan/vulkan.h>
    "./third-party/spdlog/include/spdlog/spdlog.h"
    "./third-party/spdlog/include/spdlog/fmt/fmt.h"
)
