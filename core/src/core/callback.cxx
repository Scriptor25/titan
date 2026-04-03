#include <titan/core.hxx>

#include <iostream>

void core::Application::GlfwDebugCallback(const int code, const char *description)
{
    std::cerr << std::format("[GLFW:{}] {}", code, description) << std::endl;
}

VKAPI_ATTR VkBool32 VKAPI_CALL core::Application::VkDebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const auto instance = static_cast<Application *>(user_data);
    (void) instance;

    std::cerr << std::format(
        "[{}:{}] {}",
        message_severity,
        static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(message_type),
        callback_data->pMessage) << std::endl;

    if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        __builtin_debugtrap();

    return VK_FALSE;
}

XRAPI_ATTR XrBool32 XRAPI_CALL core::Application::XrDebugCallback(
    const XrDebugUtilsMessageSeverityFlagsEXT message_severity,
    const XrDebugUtilsMessageTypeFlagsEXT message_types,
    const XrDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const auto instance = static_cast<Application *>(user_data);
    (void) instance;

    static const std::map<XrDebugUtilsMessageSeverityFlagsEXT, const char *> message_severity_map
    {
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "VERBOSE" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "INFO" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "WARNING" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "ERROR" },
    };
    static const std::map<XrDebugUtilsMessageTypeFlagsEXT, const char *> message_type_map
    {
        { XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "GENERAL" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "VALIDATION" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "PERFORMANCE" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT, "CONFORMANCE" },
    };

    std::cerr << std::format(
        "[{}:{}] {}",
        message_severity_map.at(message_severity),
        message_type_map.at(message_types),
        callback_data->message) << std::endl;

    if (message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        __builtin_debugtrap();

    return XR_FALSE;
}
