#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>
#include <titan/typename.hxx>

#include <format>
#include <vector>

namespace core
{
    namespace xr
    {
        template<typename P>
        P GetInstanceProcAddr(XrInstance instance, const char *name)
        {
            PFN_xrVoidFunction pfn;

            if (xrGetInstanceProcAddr(instance, name, &pfn))
                return nullptr;

            return reinterpret_cast<P>(pfn);
        }

        template<typename T, typename E = T, typename F, typename... A>
            requires std::is_same_v<XrResult(*)(A..., uint32_t, uint32_t *, E *), F>
        result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
        {
            uint32_t count;
            if (auto res = fn(args..., 0, &count, nullptr))
                return error<std::vector<T>>("xr::Enumerate<{}> => {}", typename_string<T>(), res);

            std::vector<T> elements(count, value);
            if (auto res = fn(args..., count, &count, reinterpret_cast<E *>(elements.data())))
                return error<std::vector<T>>("xr::Enumerate<{}> => {}", typename_string<T>(), res);

            return elements;
        }

        template<typename T = XrViewConfigurationType>
        auto EnumerateViewConfigurationTypes(
            XrInstance instance,
            XrSystemId system_id,
            const T &value = {})
        {
            return xr::Enumerate<T, XrViewConfigurationType>(
                xrEnumerateViewConfigurations,
                value,
                instance,
                system_id);
        }

        template<typename T = XrViewConfigurationView>
        auto EnumerateViewConfigurationViews(
            XrInstance instance,
            XrSystemId system_id,
            XrViewConfigurationType view_configuration_type,
            const T &value = {})
        {
            return xr::Enumerate<T, XrViewConfigurationView>(
                xrEnumerateViewConfigurationViews,
                value,
                instance,
                system_id,
                view_configuration_type);
        }

        template<typename T = int64_t>
        auto EnumerateSwapchainFormats(
            XrSession session,
            const T &value = {})
        {
            return xr::Enumerate<T, int64_t>(
                xrEnumerateSwapchainFormats,
                value,
                session);
        }

        template<typename T = XrSwapchainImageBaseHeader>
        auto EnumerateSwapchainImages(
            XrSwapchain swapchain,
            const T &value = {})
        {
            return xr::Enumerate<T, XrSwapchainImageBaseHeader>(
                xrEnumerateSwapchainImages,
                value,
                swapchain);
        }

        template<typename T = XrEnvironmentBlendMode>
        auto EnumerateEnvironmentBlendModes(
            XrInstance instance,
            XrSystemId system_id,
            XrViewConfigurationType view_configuration_type,
            const T &value = {})
        {
            return xr::Enumerate<T, XrEnvironmentBlendMode>(
                xrEnumerateEnvironmentBlendModes,
                value,
                instance,
                system_id,
                view_configuration_type);
        }
    }

    namespace vk
    {
        template<typename P>
        P GetInstanceProcAddr(VkInstance instance, const char *name)
        {
            auto pfn = vkGetInstanceProcAddr(instance, name);

            if (!pfn)
                return nullptr;

            return reinterpret_cast<P>(pfn);
        }

        template<typename P>
        P GetDeviceProcAddr(VkDevice device, const char *name)
        {
            auto pfn = vkGetDeviceProcAddr(device, name);

            if (!pfn)
                return nullptr;

            return reinterpret_cast<P>(pfn);
        }

        template<typename T, typename E = T, typename F, typename... A>
            requires std::is_same_v<VkResult (*)(A..., uint32_t *, E *), F>
        result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
        {
            uint32_t count;
            if (auto res = fn(args..., &count, nullptr))
                return error<std::vector<T>>("vk::Enumerate<{}> => {}", typename_string<T>(), res);

            std::vector<T> elements(count, value);
            if (auto res = fn(args..., &count, reinterpret_cast<E *>(elements.data())))
                return error<std::vector<T>>("vk::Enumerate<{}> => {}", typename_string<T>(), res);

            return elements;
        }

        template<typename T, typename E = T, typename F, typename... A>
            requires std::is_same_v<void (*)(A..., uint32_t *, E *), F>
        std::vector<T> Get(F fn, const T &value, A... args)
        {
            uint32_t count;
            fn(args..., &count, nullptr);

            std::vector<T> elements(count, value);
            fn(args..., &count, reinterpret_cast<E *>(elements.data()));

            return elements;
        }

        template<typename T = VkQueueFamilyProperties>
        std::vector<T> GetPhysicalDeviceQueueFamilyProperties(
            VkPhysicalDevice physical_device,
            const T &value = {})
        {
            return vk::Get<T, VkQueueFamilyProperties>(
                vkGetPhysicalDeviceQueueFamilyProperties,
                value,
                physical_device);
        }
    }
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanInstanceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t *bufferCountOutput,
    char *buffer);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanDeviceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t *bufferCountOutput,
    char *buffer);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT *createInfo,
    XrDebugUtilsMessengerEXT *messenger);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyDebugUtilsMessengerEXT(
    XrInstance instance,
    XrDebugUtilsMessengerEXT messenger);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsDeviceKHR(
    XrInstance instance,
    XrSystemId systemId,
    VkInstance vkInstance,
    VkPhysicalDevice *vkPhysicalDevice);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR *graphicsRequirements);


XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR *createInfo,
    VkInstance *vulkanInstance,
    VkResult *vulkanResult);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR *createInfo,
    VkDevice *vulkanDevice,
    VkResult *vulkanResult);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR *getInfo,
    VkPhysicalDevice *vulkanPhysicalDevice);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR *graphicsRequirements);
