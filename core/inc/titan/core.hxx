#pragma once

#include <titan/obj.hxx>
#include <titan/result.hxx>
#include <titan/wrapper/al.hxx>
#include <titan/wrapper/glfw.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <string_view>
#include <vector>

namespace core
{
    struct XrSwapchainReference
    {
        VkFormat Format{};

        xr::Swapchain Swapchain;

        std::vector<vk::DeviceMemory> Memory;
        std::vector<vk::Image> Images;
        std::vector<vk::ImageView> Views;
    };

    struct XrSwapchainView
    {
        XrViewConfigurationView View;

        XrSwapchainReference Color;
        XrSwapchainReference Depth;

        vk::CommandBuffer Buffer;
        std::vector<vk::Framebuffer> Framebuffers;
    };

    struct VkSwapchainReference
    {
        VkFormat Format{};

        vk::SwapchainKHR Swapchain;

        std::vector<vk::DeviceMemory> Memory;
        std::vector<vk::Image> Images;
        std::vector<vk::ImageView> Views;
    };

    struct VkSwapchainView
    {
        uint32_t Width{}, Height{};

        VkSwapchainReference Color;
        VkSwapchainReference Depth;
    };

    struct VkFrameInfo
    {
        vk::Semaphore Available;
        vk::Semaphore Finished;
        vk::Fence Fence;

        vk::CommandBuffer Buffer;
        vk::Framebuffer Framebuffer;
    };

    struct LayerInfo
    {
        XrTime PredictedDisplayTime;
        std::vector<XrCompositionLayerBaseHeader *> Layers;
        XrCompositionLayerProjection Projection;
        std::vector<XrCompositionLayerProjectionView> Views;
    };

    struct CameraData
    {
        glm::mat4 Screen;
        glm::mat4 Model;
        glm::mat4 Normal;
    };

    struct FormatReference
    {
        std::string_view Name;
        VkFormat &Format;
        VkImageTiling Tiling;
        VkFormatFeatureFlags2 Features;
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

    struct Pose
    {
        glm::quat Orientation{ 0.0f, 0.0f, 0.0f, 1.0f };
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
    };

    struct VkSwapchainReferenceCreateInfo
    {
        bool useSwapchain;
        uint32_t imageCount;
        VkExtent2D imageExtent;
        VkFormat imageFormat;
        VkImageAspectFlags aspectMask;
        VkImageUsageFlags imageUsage;
        VkColorSpaceKHR imageColorSpace;
        VkSharingMode imageSharingMode;
        VkPresentModeKHR presentMode;
        VkSurfaceTransformFlagBitsKHR preTransform;
        uint32_t queueFamilyIndexCount;
        const uint32_t *pQueueFamilyIndices;
    };

    struct XrSwapchainReferenceCreateInfo
    {
        bool useSwapchain;
        uint32_t sampleCount;
        XrSwapchainUsageFlags usageFlags;
        uint32_t imageCount;
        VkExtent2D imageExtent;
        VkFormat imageFormat;
        VkImageAspectFlags aspectMask;
        VkImageUsageFlags imageUsage;
        VkSharingMode imageSharingMode;
        uint32_t queueFamilyIndexCount;
        const uint32_t *pQueueFamilyIndices;
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

        static constexpr auto FOV = 110.0f;
        static constexpr auto NEAR = 0.01f;
        static constexpr auto FAR = 100.0f;

        static constexpr std::array VK_COLOR_FORMATS
        {
            VK_FORMAT_R8G8B8A8_SRGB,
        };

        static constexpr std::array VK_DEPTH_FORMATS
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
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
            const std::vector<VkFormat> &formats,
            const std::vector<FormatReference> &references);

        static result<uint32_t> FindMemoryType(
            VkPhysicalDevice physical_device,
            uint32_t type_filter,
            VkMemoryPropertyFlags type_flags);

        static std::vector<char> LoadBinary(const std::filesystem::path &path);
        static void StoreBinary(const std::filesystem::path &path, const std::vector<char> &data);

    public:
        explicit Application(ApplicationInfo info);
        virtual ~Application() = default;

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        result<> Initialize(
            std::string_view exec,
            const std::vector<std::string_view> &args);

        void Terminate() const;

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
        result<> GetFormats();
        result<> GetQueueFamilyIndices();
        result<> CreateDevice();
        result<> GetDeviceQueues();

        result<> CreateSession();

        result<> GetViewConfigurationType();
        result<> GetViewConfigurationViews();
        result<> CreateSwapchainViews();
        result<> GetEnvironmentBlendMode();
        result<> CreateReferenceSpace();

        result<> CreateWindowSurface();
        result<> CreateWindowSwapchainView();

        result<VkSwapchainReference> CreateSwapchainReference(const VkSwapchainReferenceCreateInfo &create_info);
        result<XrSwapchainReference> CreateSwapchainReference(const XrSwapchainReferenceCreateInfo &create_info);

        result<> CreateDescriptorSetLayouts();
        result<> CreateDescriptorPool();
        result<> AllocateDescriptorSets();

        result<> CreateRenderPass();
        result<> CreatePipelineCache();
        result<> StorePipelineCache();
        result<> CreatePipelineLayout();
        result<> CreatePipeline();

        result<> CreateFramebuffers();

        result<> CreateCommandPools();
        result<> AllocateCommandBuffers();

        result<> CreateSynchronization();

        result<> CreateBuffers();
        result<> AllocateBufferMemory();
        result<> FillVertexBuffer();

        result<> RecordCommandBuffer(
            uint32_t width,
            uint32_t height,
            const CameraData &camera_data,
            vk::CommandBuffer &buffer,
            vk::Framebuffer &
            framebuffer);

        result<> PollEvents();

        result<> RenderFrame();
        result<> RenderLayer(LayerInfo &reference);

        result<> UpdateModel();

        result<> RenderThirdEye(XrTime time);

    protected:
        virtual result<> OnStart();
        virtual result<> PreFrame();
        virtual result<> OnFrame();
        virtual result<> PostFrame();
        virtual result<> OnStop();

    private:
        ApplicationInfo m_Info;

        obj::Mesh m_Mesh;
        glm::mat4 m_Model{ 1.0f }, m_Normal{ 1.0f };

        glfw::Instance m_GlfwInstance;
        glfw::Window m_Window;

        al::Device m_AlDevice;
        al::Context m_AlContext;

        xr::Instance m_XrInstance;
        xr::DebugUtilsMessengerEXT m_XrMessenger;

        XrSystemId m_SystemId{};
        XrViewConfigurationType m_ViewConfigurationType{};

        vk::Instance m_VkInstance;
        vk::DebugUtilsMessengerEXT m_VkMessenger;

        VkPhysicalDevice m_PhysicalDevice{};
        vk::Device m_Device;

        QueueFamilyIndices m_QueueFamilyIndices;
        VkQueue m_DefaultQueue{}, m_TransferQueue{}, m_PresentQueue{};

        vk::CommandPool m_DefaultPool, m_TransferPool;

        vk::SurfaceKHR m_WindowSurface;
        VkSwapchainView m_WindowSwapchainView;
        std::vector<VkFrameInfo> m_Frames;
        uint32_t m_FrameIndex{};

        XrSessionState m_SessionState{};
        xr::Session m_Session;

        std::vector<XrSwapchainView> m_SwapchainViews;
        vk::Fence m_Fence;

        XrEnvironmentBlendMode m_EnvironmentBlendMode{};
        xr::ReferenceSpace m_ViewSpace, m_ReferenceSpace;

        vk::DescriptorPool m_DescriptorPool;
        std::vector<vk::DescriptorSetLayout> m_DescriptorSetLayouts;
        std::vector<vk::DescriptorSet> m_DescriptorSets;

        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipelineCache;
        vk::PipelineLayout m_PipelineLayout;
        vk::GraphicsPipeline m_Pipeline;

        vk::DeviceMemory m_VertexMemory;
        vk::Buffer m_VertexBuffer;

        Pose m_HeadPose;

        VkFormat m_ColorFormat{}, m_DepthFormat{};
    };
}
