#include <titan/core.hxx>

titan::result<> titan::Application::CreateXrMessenger()
{
    const XrDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info
    {
        .type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT,
        .userCallback = XrDebugCallback,
        .userData = this,
    };

    return xr::DebugUtilsMessengerEXT::create(m_XrInstance, debug_utils_messenger_create_info) >> m_XrMessenger;
}
