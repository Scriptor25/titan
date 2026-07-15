#pragma once

#include <titan/system/entity.hxx>
#include <titan/system/graphics.hxx>
#include <titan/system/input.hxx>
#include <titan/system/resource.hxx>
#include <titan/wrapper/al.hxx>
#include <titan/wrapper/glfw.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <filesystem>
#include <string_view>
#include <vector>

namespace titan
{
    struct VersionInfo
    {
        int Major;
        int Minor;
        int Patch;
    };

    struct ApplicationInfo
    {
        std::string Name;
        VersionInfo Version;
    };

    class Application
    {
        static constexpr auto VERSION_MAJOR = 0;
        static constexpr auto VERSION_MINOR = 0;
        static constexpr auto VERSION_PATCH = 0;

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

    public:
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

        [[nodiscard]] static toolkit::result<std::vector<char>> LoadBinary(
            const std::filesystem::path &path);

        [[nodiscard]] static toolkit::result<> StoreBinary(
            const std::filesystem::path &path,
            const std::vector<char> &data);

        explicit Application(ApplicationInfo info);
        virtual ~Application() = default;

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        [[nodiscard]] toolkit::result<> Initialize(
            std::string_view exec,
            const std::vector<std::string_view> &args);

        void Terminate() const;

        [[nodiscard]] toolkit::result<bool> Spin();
        [[nodiscard]] toolkit::result<> Destroy();

        const std::string &GetName() const;

        VersionInfo GetVersion() const;
        VersionInfo GetEngineVersion() const;

        ResourceSystem &GetResources();
        EntitySystem &GetEntities();
        InputSystem &GetInputs();
        GraphicsSystem &GetGraphics();

        glfw::Window &GetWindow();

        xr::Instance &GetXrInstance();
        XrSystemId GetXrSystemId();
        XrViewConfigurationType GetXrViewConfigurationType();
        std::span<XrViewConfigurationView> GetXrViewConfigurationViews();

        xr::Session &GetXrSession();

    private:
        [[nodiscard]] toolkit::result<> InitializeWindow();
        [[nodiscard]] toolkit::result<> InitializeAudio();
        [[nodiscard]] toolkit::result<> InitializeXr();

        [[nodiscard]] toolkit::result<> InitializeXrInstance();
        [[nodiscard]] toolkit::result<> InitializeXrMessenger();
        [[nodiscard]] toolkit::result<> InitializeXrSystemId();

        [[nodiscard]] toolkit::result<> InitializeXrViewConfigurationType();
        [[nodiscard]] toolkit::result<> InitializeXrViewConfigurationViews();
        [[nodiscard]] toolkit::result<> InitializeXrEnvironmentBlendMode();

        [[nodiscard]] toolkit::result<bool> PollEvents();

        [[nodiscard]] toolkit::result<> RenderFrame();

        [[nodiscard]] toolkit::result<> UpdateEntities();
        [[nodiscard]] toolkit::result<> UpdateInputs(XrTime time);

    protected:
        [[nodiscard]] virtual toolkit::result<> OnInitialize();
        [[nodiscard]] virtual toolkit::result<> OnStart();
        [[nodiscard]] virtual toolkit::result<> PreFrame();
        [[nodiscard]] virtual toolkit::result<> OnFrame();
        [[nodiscard]] virtual toolkit::result<> PostFrame();
        [[nodiscard]] virtual toolkit::result<> OnStop();

    private:
        ApplicationInfo m_Info;

        Heap m_Heap;

        glfw::Instance m_GlfwInstance;
        glfw::Window m_Window;

        al::Device m_AlDevice;
        al::Context m_AlContext;

        xr::Instance m_XrInstance;
        xr::DebugUtilsMessengerEXT m_XrMessenger;
        XrSystemId m_XrSystemId{};
        XrViewConfigurationType m_XrViewConfigurationType{};
        std::vector<XrViewConfigurationView> m_XrViewConfigurationViews;
        XrEnvironmentBlendMode m_XrEnvironmentBlendMode{};

        XrSessionState m_XrSessionState{};

        ResourceSystem m_Resources;
        EntitySystem m_Entities;
        GraphicsSystem m_Graphics;
        InputSystem m_Inputs;
    };
}
