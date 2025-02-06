cmake_minimum_required(VERSION 3.20)

file(GLOB third-party-srcs CONFIGURE_DEPENDS
    "./*.cpp"
    "./spirv-reflect/spirv_reflect.cpp"
    "./tinyobjloader/tiny_obj_loader.cc"
)

target_sources(vulkan-renderer PUBLIC ${third-party-srcs})

# GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# spdlog
add_subdirectory(spdlog)
