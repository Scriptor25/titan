#pragma once

#include <titan/result.hxx>
#include <titan/system/entity.hxx>
#include <titan/system/graphics.hxx>
#include <titan/system/input.hxx>
#include <titan/system/resource.hxx>
#include <titan/wrapper/al.hxx>
#include <titan/wrapper/glfw.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <string_view>
#include <vector>

namespace titan
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

        std::vector<vk::Framebuffer> Framebuffers;
        vk::CommandBuffer Buffer;
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

        vk::Framebuffer Framebuffer;
        vk::CommandBuffer Buffer;
    };

    struct LayerInfo
    {
        XrTime PredictedDisplayTime;
        std::vector<XrCompositionLayerBaseHeader *> Layers;
        XrCompositionLayerProjection Projection;
        std::vector<XrCompositionLayerProjectionView> Views;
    };

    struct ShaderData
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

    struct ModelPose
    {
        glm::quat Orientation{ 1.0f, 0.0f, 0.0f, 0.0f };
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

    struct HandState
    {
        XrPath Path{};
        xr::ActionSpace Space;

        XrActionStateFloat GrabState{ XR_TYPE_ACTION_STATE_FLOAT };
        XrActionStatePose PoseState{ XR_TYPE_ACTION_STATE_POSE };

        float Haptic{};

        ModelPose Pose;
    };

    struct ModelMatrices
    {
        glm::mat4 Model{ 1.0f }, Normal{ 1.0f };
    };

    struct ModelData
    {
        ResourceID Mesh{};
        uint32_t InstanceCount{};
    };

    struct ModelReference
    {
        glm::vec3 BoxMin, BoxMax, BoxCen;

        vk::DeviceMemory VertexMemory;
        vk::Buffer VertexBuffer;

        VkDeviceSize VertexBufferOffset;
        VkDeviceSize VertexBufferSize;
        VkDeviceSize VertexBufferStride;

        vk::DeviceMemory IndexMemory;
        vk::Buffer IndexBuffer;

        VkDeviceSize IndexBufferOffset;
        VkDeviceSize IndexBufferSize;
        VkIndexType IndexType;

        uint32_t IndexCount;

        std::vector<ModelMatrices> Instances;
        std::vector<bool> Active;
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

        static toolkit::result<> FindFormats(
            VkPhysicalDevice physical_device,
            const std::vector<VkFormat> &formats,
            const std::vector<FormatReference> &references);

        static toolkit::result<uint32_t> FindMemoryType(
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

        toolkit::result<> Initialize(
            std::string_view exec,
            const std::vector<std::string_view> &args);

        void Terminate() const;

        toolkit::result<bool> Spin();
        toolkit::result<> CleanUp();

        ResourceSystem &GetResources();
        EntitySystem &GetEntities();
        InputSystem &GetInputs();
        GraphicsSystem &GetGraphics();

    private:
        toolkit::result<> InitializeWindow();
        toolkit::result<> InitializeAudio();
        toolkit::result<> InitializeGraphics();

        toolkit::result<> CreateXrInstance();
        toolkit::result<> CreateXrMessenger();
        toolkit::result<> GetSystemId();

        toolkit::result<> CreateActionSet();

        toolkit::result<> CreateActions();
        toolkit::result<xr::Action> CreateAction(
            const std::string &name,
            const std::string &localized_name,
            XrActionType type,
            const std::vector<std::string> &sub_path_strings = {});

        toolkit::result<> CreateHands();

        toolkit::result<> SuggestBindings();
        toolkit::result<> RecordBindings();

        toolkit::result<> CreateVkInstance();
        toolkit::result<> CreateVkMessenger();

        toolkit::result<> GetPhysicalDevice();
        toolkit::result<> GetFormats();
        toolkit::result<> GetQueueFamilyIndices();
        toolkit::result<> CreateDevice();
        toolkit::result<> GetDeviceQueues();

        toolkit::result<> CreateSession();

        toolkit::result<> CreateActionSpaces();
        toolkit::result<xr::ActionSpace> CreateActionSpace(
            XrAction action,
            const std::optional<std::string> &sub_path_string = std::nullopt);

        toolkit::result<> AttachActionSet();

        toolkit::result<> GetViewConfigurationType();
        toolkit::result<> GetViewConfigurationViews();
        toolkit::result<> CreateSwapchainViews();
        toolkit::result<> GetEnvironmentBlendMode();
        toolkit::result<> CreateReferenceSpace();

        toolkit::result<> CreateWindowSurface();
        toolkit::result<> CreateWindowSwapchainView();

        toolkit::result<VkSwapchainReference> CreateSwapchainReference(
            const VkSwapchainReferenceCreateInfo &create_info);
        toolkit::result<XrSwapchainReference> CreateSwapchainReference(
            const XrSwapchainReferenceCreateInfo &create_info);

        toolkit::result<> CreateRenderPass();
        toolkit::result<> CreatePipelineCache();
        toolkit::result<> StorePipelineCache();
        toolkit::result<> CreatePipelineLayout();
        toolkit::result<> CreatePipeline();

        toolkit::result<> CreateFramebuffers();

        toolkit::result<> CreateCommandPools();
        toolkit::result<> AllocateCommandBuffers();

        toolkit::result<> CreateSynchronization();

        toolkit::result<> CreateBuffers();
        toolkit::result<> FillBuffers();

        toolkit::result<> RecordCommandBuffer(
            uint32_t width,
            uint32_t height,
            const glm::mat4 &screen_matrix,
            vk::CommandBuffer &buffer,
            vk::Framebuffer &
            framebuffer);

        toolkit::result<bool> PollEvents();
        toolkit::result<> PollActions(XrTime time);

        toolkit::result<> RenderFrame();
        toolkit::result<> RenderLayer(LayerInfo &reference);

        toolkit::result<> UpdateModels();
        toolkit::result<> Interaction();

        toolkit::result<> RenderThirdEye(XrTime time);

    protected:
        virtual toolkit::result<> OnStart();
        virtual toolkit::result<> PreFrame();
        virtual toolkit::result<> OnFrame();
        virtual toolkit::result<> PostFrame();
        virtual toolkit::result<> OnStop();

    private:
        ApplicationInfo m_Info;
        std::vector<ModelData> m_ModelData;

        ResourceSystem m_Resources;
        EntitySystem m_Entities;
        InputSystem m_Inputs;
        GraphicsSystem m_Graphics;

        ModelPose m_HeadPose;

        VkFormat m_ColorFormat{}, m_DepthFormat{};

        glfw::Instance m_GlfwInstance;
        glfw::Window m_Window;

        al::Device m_AlDevice;
        al::Context m_AlContext;

        xr::Instance m_XrInstance;
        xr::DebugUtilsMessengerEXT m_XrMessenger;

        XrSystemId m_SystemId{};
        XrViewConfigurationType m_ViewConfigurationType{};

        xr::ActionSet m_ActionSet;
        xr::Action m_ActionGrab, m_ActionHaptic, m_ActionPalmPose;

        vk::Instance m_VkInstance;
        vk::DebugUtilsMessengerEXT m_VkMessenger;

        VkPhysicalDevice m_PhysicalDevice{};
        vk::Device m_Device;

        QueueFamilyIndices m_QueueFamilyIndices;
        VkQueue m_DefaultQueue{}, m_TransferQueue{}, m_PresentQueue{};

        vk::CommandPool m_DefaultPool, m_TransferPool;

        vk::SurfaceKHR m_WindowSurface;
        VkSwapchainView m_WindowSwapchainView;

        XrSessionState m_SessionState{};
        xr::Session m_Session;

        XrEnvironmentBlendMode m_EnvironmentBlendMode{};
        xr::ReferenceSpace m_ViewSpace, m_ReferenceSpace;

        std::array<HandState, 2> m_Hands;

        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipelineCache;
        vk::PipelineLayout m_PipelineLayout;
        vk::GraphicsPipeline m_Pipeline;

        std::vector<ModelReference> m_ModelReferences;

        std::vector<XrSwapchainView> m_SwapchainViews;
        vk::Fence m_Fence;

        std::vector<VkFrameInfo> m_Frames;
        uint32_t m_FrameIndex{};
    };
}
