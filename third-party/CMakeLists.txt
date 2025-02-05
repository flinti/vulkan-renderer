cmake_minimum_required(VERSION 3.20)

file(GLOB third-party-srcs CONFIGURE_DEPENDS
    "./*.cpp"
    "./spirv-reflect/spirv_reflect.cpp"
    "./tinyobjloader/tiny_obj_loader.cc"
)

target_sources(vulkan-renderer PUBLIC ${third-party-srcs})
