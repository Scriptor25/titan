#pragma once

#include <titan/api.hxx>
#include <titan/system/resource.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <glm/glm.hpp>

namespace titan
{
    namespace detail
    {
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

        struct MeshData
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
        };

        struct QueueFamilyIndices
        {
            uint32_t Default{};
            uint32_t Graphics{};
            uint32_t Compute{};
            uint32_t Transfer{};
            uint32_t Present{};
        };

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
            XrViewConfigurationView ViewConfigurationView;

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

        struct VkFrameReference
        {
            vk::Semaphore Available;
            vk::Semaphore Finished;
            vk::Fence Fence;

            vk::Framebuffer Framebuffer;
            vk::CommandBuffer Buffer;
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

        struct LayerReference
        {
            XrTime PredictedDisplayTime;
            std::vector<XrCompositionLayerBaseHeader *> Layers;
            XrCompositionLayerProjection Projection;
            std::vector<XrCompositionLayerProjectionView> Views;
        };

        struct RecordCommandBufferInfo
        {
            uint32_t width;
            uint32_t height;

            const glm::mat4 &screen_matrix;

            vk::CommandBuffer &buffer;
            vk::Framebuffer &framebuffer;
        };
    }

    class GraphicsSystem
    {
        friend class Application;

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

        static constexpr auto FOV = 110.0f;
        static constexpr auto NEAR = 0.01f;
        static constexpr auto FAR = 100.0f;

        static void GetInstanceExtensions(std::vector<const char *> &dst);
        static void GetDeviceExtensions(std::vector<const char *> &dst);

        static toolkit::result<> FindFormats(
            VkPhysicalDevice physical_device,
            const std::vector<VkFormat> &formats,
            const std::vector<detail::FormatReference> &references);

        static toolkit::result<uint32_t> FindMemoryType(
            VkPhysicalDevice physical_device,
            uint32_t type_filter,
            VkMemoryPropertyFlags type_flags);

    public:
        GraphicsSystem(Application &application, Heap &heap);
        ~GraphicsSystem();

        [[nodiscard]] toolkit::result<> Initialize();
        [[nodiscard]] toolkit::result<> Destroy();

        [[nodiscard]] toolkit::result<> RenderFrame();

    protected:
        [[nodiscard]] toolkit::result<> CreateInstance();
        [[nodiscard]] toolkit::result<> CreateMessenger();

        [[nodiscard]] toolkit::result<> CreatePhysicalDevice();
        [[nodiscard]] toolkit::result<> CreateFormats();
        [[nodiscard]] toolkit::result<> CreateWindowSurface();
        [[nodiscard]] toolkit::result<> CreateQueueFamilyIndices();

        [[nodiscard]] toolkit::result<> CreateDevice();
        [[nodiscard]] toolkit::result<> CreateDeviceQueues();

        [[nodiscard]] toolkit::result<> CreateWindowSwapchainView();

        [[nodiscard]] toolkit::result<> CreateSession();
        [[nodiscard]] toolkit::result<> CreateSpaces();

        [[nodiscard]] toolkit::result<> CreateCommandPools();

        [[nodiscard]] toolkit::result<> CreateRenderPass();

        [[nodiscard]] toolkit::result<> CreatePipelineCache();
        [[nodiscard]] toolkit::result<> CreatePipelineLayout();
        [[nodiscard]] toolkit::result<> CreatePipeline();

        [[nodiscard]] toolkit::result<> CreateBuffers();
        [[nodiscard]] toolkit::result<> FillBuffers();

        [[nodiscard]] toolkit::result<> CreateSwapchainViews();
        [[nodiscard]] toolkit::result<> CreateFrames();

        [[nodiscard]] toolkit::result<> CreateSynchronization();

        [[nodiscard]] toolkit::result<> StorePipelineCache();

        [[nodiscard]] toolkit::result<detail::VkSwapchainReference> CreateSwapchainReference(
            const detail::VkSwapchainReferenceCreateInfo &create_info);
        [[nodiscard]] toolkit::result<detail::XrSwapchainReference> CreateSwapchainReference(
            const detail::XrSwapchainReferenceCreateInfo &create_info);

        [[nodiscard]] toolkit::result<> RenderWindowView();
        [[nodiscard]] toolkit::result<> RenderViews(detail::LayerReference &reference);

        [[nodiscard]] toolkit::result<> RecordCommandBuffer(const detail::RecordCommandBufferInfo &info);

    private:
        Application &m_Application;

        Heap &m_Heap;

        VkFormat m_ColorFormat{}, m_DepthFormat{};

        vk::Instance m_Instance;
        vk::DebugUtilsMessengerEXT m_Messenger;

        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_Device;

        vk::SurfaceKHR m_WindowSurface;
        detail::VkSwapchainView m_WindowSwapchainView;

        detail::QueueFamilyIndices m_QueueFamilyIndices;
        vk::Queue m_DefaultQueue, m_TransferQueue, m_PresentQueue;

        xr::Session m_Session;
        xr::ReferenceSpace m_ViewSpace, m_ReferenceSpace;

        vk::CommandPool m_DefaultPool, m_TransferPool;

        vk::RenderPass m_RenderPass;

        vk::PipelineCache m_PipelineCache;
        vk::PipelineLayout m_PipelineLayout;
        vk::GraphicsPipeline m_Pipeline;

        std::vector<detail::XrSwapchainView> m_SwapchainViews;
        vk::Fence m_Fence;

        std::vector<detail::VkFrameReference> m_Frames;
        uint32_t m_FrameIndex{};

        std::unordered_map<ResourceID, detail::MeshData> m_Meshes;
    };
}
