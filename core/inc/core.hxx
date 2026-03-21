#pragma once

#include <string_view>
#include <vector>

#include <result.hxx>
#include <wrapper/al.hxx>
#include <wrapper/glfw.hxx>
#include <wrapper/vk.hxx>
#include <wrapper/xr.hxx>

namespace core
{
    struct SwapchainReference
    {
        VkFormat format;
        xr::Swapchain swapchain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> views;
    };

    struct LayerReference
    {
        XrTime predicted_display_time;
        std::vector<XrCompositionLayerBaseHeader *> layers;
        XrCompositionLayerProjection projection;
        std::vector<XrCompositionLayerProjectionView> projection_views;
    };

    class Instance
    {
        static constexpr std::array VIEW_CONFIGURATION_TYPES
        {
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO,
        };

        static constexpr std::array ENVIRONMENT_BLEND_MODES
        {
            XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND,
            XR_ENVIRONMENT_BLEND_MODE_ADDITIVE,
            XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        };

    public:
        Instance() = default;
        ~Instance() = default;

        Instance(const Instance &) = delete;
        Instance &operator=(const Instance &) = delete;

        result<> Initialize(
            std::string_view exec,
            const std::vector<std::string_view> &args);

        void Terminate();

        result<> Spin();

    protected:
        result<> InitializeWindow();
        result<> InitializeAudio();
        result<> InitializeGraphics();

        result<> CreateXrInstance();
        result<> CreateXrMessenger();
        result<> GetSystemId();

        result<> CreateVkInstance();
        result<> CreateVkMessenger();

        result<> GetPhysicalDevice();
        result<> CreateDevice();
        result<> GetQueueFamilyIndex();
        result<> GetQueueIndex();

        result<> CreateSession();

        result<> GetViewConfigurationType();
        result<> GetViewConfigurationViews();
        result<> GetFormats();
        result<> CreateSwapchains();
        result<> GetEnvironmentBlendMode();
        result<> CreateReferenceSpace();

        result<> CreateSurface();

        result<SwapchainReference> CreateSwapchain(
            const XrViewConfigurationView &view_configuration_view,
            XrSwapchainUsageFlags usage_flags,
            VkFormat format,
            VkImageAspectFlags aspect_mask);

        result<> PollEvents();

        result<> RenderFrame();
        result<> RenderLayer(LayerReference &reference);

    private:
        glfw::Library m_GlfwLibrary;
        glfw::Window m_GlfwWindow;

        al::Device m_AlDevice;
        al::Context m_AlContext;

        xr::Instance m_XrInstance;
        xr::DebugUtilsMessengerEXT m_XrMessenger;

        xr::SystemId m_SystemId;

        XrViewConfigurationType m_ViewConfigurationType{};
        std::vector<XrViewConfigurationView> m_ViewConfigurationViews;

        vk::Instance m_VkInstance;
        vk::DebugUtilsMessengerEXT m_VkMessenger;

        VkPhysicalDevice m_PhysicalDevice{};
        vk::Device m_Device;
        std::uint32_t m_QueueFamilyIndex = UINT32_MAX, m_QueueIndex = UINT32_MAX;

        vk::GLFWSurface m_Surface;

        XrSessionState m_SessionState{};
        xr::Session m_Session;

        VkFormat m_ColorFormat{}, m_DepthFormat{};
        std::vector<SwapchainReference> m_ColorSwapchainReferences, m_DepthSwapchainReferences;

        XrEnvironmentBlendMode m_EnvironmentBlendMode{};
        xr::ReferenceSpace m_ReferenceSpace;
    };
}
