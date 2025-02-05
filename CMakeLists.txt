cmake_minimum_required(VERSION 3.20)
project(vulkan)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD, "20")
set(CMAKE_CXX_STANDARD_REQUIRED, ON)

add_compile_definitions(GLFW_INCLUDE_VULKAN)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
if(NOT Vulkan_glslc_FOUND)
    message(FATAL_ERROR "glslc is required to be installed!")
endif()

link_libraries(spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
link_libraries(glfw)
link_libraries(Vulkan::Vulkan)

file(GLOB vulkan-renderer-srcs CONFIGURE_DEPENDS
    "./src/*.cpp"
)

add_executable(vulkan-renderer ${vulkan-renderer-srcs})
add_subdirectory(third-party)

target_precompile_headers(vulkan-renderer
    PUBLIC
    <vulkan/vulkan_core.h>
    <spdlog/spdlog.h>
    <spdlog/fmt/fmt.h>
)
