#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED

#include <titan/obj.hxx>
#include <titan/result.hxx>
#include <titan/wrapper/al.hxx>
#include <titan/wrapper/glfw.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <glm/glm.hpp>

#include <filesystem>
#include <string_view>
#include <vector>

namespace core
{
    struct SwapchainReference
    {
        VkFormat Format;
        xr::Swapchain Swapchain;
        std::vector<vk::Image> Images;
        std::vector<vk::ImageView> Views;
    };

    struct SwapchainFrame
    {
        SwapchainReference Color;
        SwapchainReference Depth;
        std::vector<vk::Framebuffer> Framebuffers;
    };

    struct LayerReference
    {
        XrTime PredictedDisplayTime;
        std::vector<XrCompositionLayerBaseHeader *> Layers;
        XrCompositionLayerProjection Projection;
        std::vector<XrCompositionLayerProjectionView> Views;
    };

    struct CameraData
    {
        glm::mat4 model;
        glm::mat4 inv_model;
        glm::mat4 view;
        glm::mat4 inv_view;
        glm::mat4 proj;
        glm::mat4 inv_proj;
    };

    struct FormatReference
    {
        std::string_view name;
        VkFormat &format;
        VkFormatFeatureFlags features;
    };

    class Instance
    {
        static constexpr std::array VK_LAYERS
        {
            "VK_LAYER_KHRONOS_validation",
        };

        static constexpr std::array VK_INSTANCE_EXTENSIONS
        {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        };

        static constexpr std::array VK_DEVICE_EXTENSIONS
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        };

        static constexpr std::array XR_INSTANCE_EXTENSIONS
        {
            XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
            XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
            XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
        };

        static constexpr std::array XR_VIEW_CONFIGURATION_TYPES
        {
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO,
        };

        static constexpr std::array XR_ENVIRONMENT_BLEND_MODES
        {
            XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND,
            XR_ENVIRONMENT_BLEND_MODE_ADDITIVE,
            XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        };

        static void GlfwDebugCallback(int code, const char *description);

        static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
            void *user_data);

        static XRAPI_ATTR XrBool32 XRAPI_CALL XrDebugCallback(
            XrDebugUtilsMessageSeverityFlagsEXT message_severity,
            XrDebugUtilsMessageTypeFlagsEXT message_types,
            const XrDebugUtilsMessengerCallbackDataEXT *callback_data,
            void *user_data);

        static void GetInstanceExtensions(std::vector<const char *> &dst);
        static void GetDeviceExtensions(std::vector<const char *> &dst);

        static result<> FindFormats(
            VkPhysicalDevice physical_device,
            const std::vector<int64_t> &formats,
            const std::vector<FormatReference> &references);

        static result<uint32_t> FindMemoryType(
            VkPhysicalDevice physical_device,
            uint32_t type_filter,
            VkMemoryPropertyFlags type_flags);

        static std::vector<char> LoadShaderModuleBinary(const std::filesystem::path &path);

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
            const XrViewConfigurationView &view,
            XrSwapchainUsageFlags usage,
            VkFormat format,
            VkImageAspectFlags aspect);

        result<> CreateDescriptorSetLayout();
        result<> CreateDescriptorPool();
        result<> CreateDescriptorSet();

        result<> CreateRenderPass();
        result<> CreatePipelineCache();
        result<> CreatePipelineLayout();
        result<> CreatePipeline();

        result<> CreateFramebuffers();

        result<> CreateCommandPool();
        result<> CreateCommandBuffer();

        result<> CreateSynchronization();

        result<> CreateVertexBuffer();
        result<> CreateVertexMemory();
        result<> FillVertexBuffer();

        result<> CreateCameraBuffer();
        result<> CreateCameraMemory();

        result<> RecordCommandBuffer(uint32_t view_index, uint32_t image_index);

        result<> PollEvents();

        result<> RenderFrame();
        result<> RenderLayer(LayerReference &reference);

    private:
        obj::Mesh m_Mesh;
        CameraData m_CameraData{};

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
        uint32_t m_QueueFamilyIndex = UINT32_MAX, m_QueueIndex = UINT32_MAX;

        vk::GLFWSurface m_Surface;

        XrSessionState m_SessionState{};
        xr::Session m_Session;

        VkFormat m_ColorFormat{}, m_DepthFormat{};
        std::vector<SwapchainFrame> m_SwapchainFrames;

        XrEnvironmentBlendMode m_EnvironmentBlendMode{};
        xr::ReferenceSpace m_ReferenceSpace;

        vk::DescriptorSetLayout m_DescriptorSetLayout;
        vk::DescriptorPool m_DescriptorPool;
        vk::DescriptorSet m_DescriptorSet;

        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipelineCache;
        vk::PipelineLayout m_PipelineLayout;
        vk::Pipeline m_Pipeline;

        vk::CommandPool m_CommandPool;
        vk::CommandBuffer m_CommandBuffer;

        vk::Semaphore m_ImageAvailableSemaphore, m_RenderFinishedSemaphore;
        vk::Fence m_InFlightFence;

        vk::Buffer m_VertexBuffer;
        vk::DeviceMemory m_VertexMemory;

        vk::Buffer m_CameraBuffer;
        vk::DeviceMemory m_CameraMemory;
    };
}
