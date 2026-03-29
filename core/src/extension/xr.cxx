#include <titan/extension.hxx>

XRAPI_ATTR XrResult XRAPI_CALL xrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT *createInfo,
    XrDebugUtilsMessengerEXT *messenger)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrCreateDebugUtilsMessengerEXT>(
        instance,
        "xrCreateDebugUtilsMessengerEXT");

    return pfn
               ? pfn(instance, createInfo, messenger)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyDebugUtilsMessengerEXT(
    XrInstance instance,
    XrDebugUtilsMessengerEXT messenger)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrDestroyDebugUtilsMessengerEXT>(
        instance,
        "xrDestroyDebugUtilsMessengerEXT");

    return pfn
               ? pfn(messenger)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR *createInfo,
    VkInstance *vulkanInstance,
    VkResult *vulkanResult)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrCreateVulkanInstanceKHR>(
        instance,
        "xrCreateVulkanInstanceKHR");

    return pfn
               ? pfn(instance, createInfo, vulkanInstance, vulkanResult)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR *createInfo,
    VkDevice *vulkanDevice,
    VkResult *vulkanResult)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrCreateVulkanDeviceKHR>(
        instance,
        "xrCreateVulkanDeviceKHR");

    return pfn
               ? pfn(instance, createInfo, vulkanDevice, vulkanResult)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR *getInfo,
    VkPhysicalDevice *vulkanPhysicalDevice)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrGetVulkanGraphicsDevice2KHR>(
        instance,
        "xrGetVulkanGraphicsDevice2KHR");

    return pfn
               ? pfn(instance, getInfo, vulkanPhysicalDevice)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR *graphicsRequirements)
{
    static const auto pfn = core::xr::GetInstanceProcAddr<PFN_xrGetVulkanGraphicsRequirements2KHR>(
        instance,
        "xrGetVulkanGraphicsRequirements2KHR");

    return pfn
               ? pfn(instance, systemId, graphicsRequirements)
               : XR_ERROR_EXTENSION_NOT_PRESENT;
}
