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

# vulkan-renderer executable

file(GLOB VULKAN_RENDERER_SRCS CONFIGURE_DEPENDS
    "./src/*.cpp"
)

add_executable(vulkan-renderer "${VULKAN_RENDERER_SRCS}")

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


# shader compilation

set(SHADER_SRC_DIR "./data/shader-src")
set(SHADER_BIN_DIR "./data/shader")

file(GLOB SHADER_SRCS CONFIGURE_DEPENDS
    "${SHADER_SRC_DIR}/*.vert"
    "${SHADER_SRC_DIR}/*.frag"
)

foreach(FILE IN LISTS SHADER_SRCS)
    get_filename_component(SHADER_FILE_BASENAME "${FILE}" NAME)
    set(BINARY_FILE "${SHADER_BIN_DIR}/${SHADER_FILE_BASENAME}.spv")

    add_custom_command(
        OUTPUT "${BINARY_FILE}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -o "${BINARY_FILE}" "${FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        MAIN_DEPENDENCY "${FILE}"
        COMMENT "compiling shader ${FILE} to ${BINARY_FILE}" 
        VERBATIM
        COMMAND_EXPAND_LISTS
    )

    list(APPEND SHADER_BINARIES "${BINARY_FILE}")
endforeach()

add_custom_target(shaders DEPENDS "${SHADER_BINARIES}")
add_dependencies(vulkan-renderer shaders)
