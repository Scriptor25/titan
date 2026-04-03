#include <titan/core.hxx>

core::result<> core::Application::CreateSession()
{
    const XrGraphicsBindingVulkan2KHR graphics_binding_vulkan
    {
        .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
        .instance = m_VkInstance,
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .queueFamilyIndex = m_QueueFamilyIndices.Default,
        .queueIndex = 0,
    };

    const XrSessionCreateInfo session_create_info
    {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &graphics_binding_vulkan,
        .systemId = m_SystemId,
    };

    return xr::Session::create(m_XrInstance, session_create_info) >> m_Session;
}
