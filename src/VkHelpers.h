#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <stdexcept>
#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"

#define VK_ASSERT_IMPL(x, file, line) do { \
    VkResult result = x; \
    if (result != VK_SUCCESS) { \
        throw std::runtime_error( \
            fmt::format(file ": " #line "vulkan error {}: " #x, string_VkResult(result))); \
    } \
} while(0)

#define VK_ASSERT(x) VK_ASSERT_IMPL(x, __FILE__, __LINE__)
