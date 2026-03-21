#include <extension.hxx>

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger)
{
    static const auto pfn = core::vk::GetInstanceProcAddr<PFN_vkCreateDebugUtilsMessengerEXT>(
        instance,
        "vkCreateDebugUtilsMessengerEXT");

    return pfn
               ? pfn(instance, pCreateInfo, pAllocator, pMessenger)
               : VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks *pAllocator)
{
    static const auto pfn = core::vk::GetInstanceProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>(
        instance,
        "vkDestroyDebugUtilsMessengerEXT");

    return pfn
               ? pfn(instance, messenger, pAllocator)
               : (void) 0;
}
