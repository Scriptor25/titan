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
    struct XrSwapchainInfo
    {
        VkFormat Format{};
        xr::Swapchain Swapchain;
        std::vector<vk::Image> Images;
        std::vector<vk::ImageView> Views;
    };

    struct XrSwapchainView
    {
        XrSwapchainInfo Color;
        XrSwapchainInfo Depth;
        vk::CommandBuffer Buffer;
        std::vector<vk::Framebuffer> Framebuffers;
    };

    struct VkSwapchainInfo
    {
        VkFormat Format{};
        uint32_t Width{}, Height{};
        vk::SwapchainKHR Swapchain;
        std::vector<vk::Image> Images;
    };

    struct RenderLayerInfo
    {
        XrTime PredictedDisplayTime;
        std::vector<XrCompositionLayerBaseHeader *> Layers;
        XrCompositionLayerProjection Projection;
        std::vector<XrCompositionLayerProjectionView> Views;
    };

    struct CameraData
    {
        glm::mat4 Screen;
        glm::mat4 Normal;
    };

    struct FormatReference
    {
        std::string_view Name;
        VkFormat &Format;
        VkFormatFeatureFlags Features;
    };

    struct ApplicationVersion
    {
        int Major;
        int Minor;
        int Patch;
    };

    struct ApplicationInfo
    {
        std::string Name;
        ApplicationVersion Version;
    };

    struct QueueFamilyIndices
    {
        uint32_t Default{};
        uint32_t Graphics{};
        uint32_t Compute{};
        uint32_t Transfer{};
        uint32_t Present{};
    };

    struct BlitViewInfo
    {
        uint32_t Index;
        uint32_t Width;
        uint32_t Height;
    };

    struct Frame
    {
        vk::Semaphore Available;
        vk::Semaphore Finished;
        vk::Fence Fence;
        vk::CommandBuffer Buffer;
    };

    class Application
    {
        static constexpr auto VERSION_MAJOR = 0;
        static constexpr auto VERSION_MINOR = 0;
        static constexpr auto VERSION_PATCH = 0;

        static constexpr std::array VK_INSTANCE_LAYERS
        {
            "VK_LAYER_KHRONOS_validation",
        };

        static constexpr std::array<const char *, 0> VK_DEVICE_LAYERS
        {
        };

        static constexpr std::array VK_INSTANCE_EXTENSIONS
        {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
        };

        static constexpr std::array VK_DEVICE_EXTENSIONS
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
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
        explicit Application(ApplicationInfo info);
        virtual ~Application() = default;

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        result<> Initialize(
            std::string_view exec,
            const std::vector<std::string_view> &args);

        void Terminate();

        result<bool> Spin();

    private:
        result<> InitializeWindow();
        result<> InitializeAudio();
        result<> InitializeGraphics();

        result<> CreateXrInstance();
        result<> CreateXrMessenger();
        result<> GetSystemId();

        result<> CreateVkInstance();
        result<> CreateVkMessenger();

        result<> GetPhysicalDevice();
        result<> GetQueueFamilyIndices();
        result<> CreateDevice();
        result<> GetDeviceQueues();

        result<> CreateSession();

        result<> GetViewConfigurationType();
        result<> GetViewConfigurationViews();
        result<> GetFormats();
        result<> CreateSwapchains();
        result<> GetEnvironmentBlendMode();
        result<> CreateReferenceSpace();

        result<> CreateWindowSurface();
        result<> CreateWindowSwapchain();

        result<XrSwapchainInfo> CreateSwapchain(
            const XrViewConfigurationView &view,
            XrSwapchainUsageFlags usage,
            VkFormat format,
            VkImageAspectFlags aspect);

        result<> CreateDescriptorSetLayouts();
        result<> CreateDescriptorPool();
        result<> CreateDescriptorSet();

        result<> CreateRenderPass();
        result<> CreatePipelineCache();
        result<> CreatePipelineLayout();
        result<> CreatePipeline();

        result<> CreateFramebuffers();

        result<> CreateCommandPools();
        result<> CreateCommandBuffers();

        result<> CreateSynchronization();

        result<> CreateVertexBuffer();
        result<> CreateVertexMemory();
        result<> FillVertexBuffer();

        result<> RecordCommandBuffer(uint32_t view_index, uint32_t image_index, const CameraData &camera_data);

        result<> PollEvents();

        result<> RenderFrame();
        result<> RenderLayer(RenderLayerInfo &reference);

        result<> BlitView(const BlitViewInfo &info);

    protected:
        virtual void OnStart();
        virtual void PreFrame();
        virtual void OnFrame();
        virtual void PostFrame();
        virtual void OnStop();

    private:
        ApplicationInfo m_Info;

        obj::Mesh m_Mesh;

        glfw::Instance m_GlfwInstance;
        glfw::Window m_Window;

        al::Device m_AlDevice;
        al::Context m_AlContext;

        xr::Instance m_XrInstance;
        xr::DebugUtilsMessengerEXT m_XrMessenger;

        XrSystemId m_SystemId{};

        XrViewConfigurationType m_ViewConfigurationType{};
        std::vector<XrViewConfigurationView> m_ViewConfigurationViews;

        vk::Instance m_VkInstance;
        vk::DebugUtilsMessengerEXT m_VkMessenger;

        VkPhysicalDevice m_PhysicalDevice{};
        vk::Device m_Device;

        QueueFamilyIndices m_QueueFamilyIndices;
        VkQueue m_DefaultQueue{}, m_TransferQueue{}, m_PresentQueue{};

        vk::SurfaceKHR m_WindowSurface;
        VkSwapchainInfo m_WindowSwapchain;

        XrSessionState m_SessionState{};
        xr::Session m_Session;

        VkFormat m_ColorFormat{}, m_DepthFormat{};
        std::vector<XrSwapchainView> m_SwapchainViews;

        XrEnvironmentBlendMode m_EnvironmentBlendMode{};
        xr::ReferenceSpace m_ReferenceSpace;

        vk::DescriptorPool m_DescriptorPool;
        std::vector<vk::DescriptorSetLayout> m_DescriptorSetLayouts;
        std::vector<vk::DescriptorSet> m_DescriptorSets;

        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipelineCache;
        vk::PipelineLayout m_PipelineLayout;
        vk::Pipeline m_Pipeline;

        vk::CommandPool m_DefaultPool, m_TransferPool;

        vk::Fence m_Fence{};
        std::vector<Frame> m_Frames{ 4 };
        uint32_t m_FrameIndex{};

        vk::Buffer m_VertexBuffer;
        vk::DeviceMemory m_VertexMemory;
    };
}
