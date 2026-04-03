#include <titan/core.hxx>

#include <cstring>

core::result<> core::Application::CreateXrInstance()
{
    XrApplicationInfo application_info
    {
        .applicationName = {},
        .applicationVersion = static_cast<uint32_t>(XR_MAKE_VERSION(
            m_Info.Version.Major,
            m_Info.Version.Minor,
            m_Info.Version.Patch)),
        .engineName = "Titan Core",
        .engineVersion = XR_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH),
        .apiVersion = XR_CURRENT_API_VERSION,
    };

    std::memcpy(application_info.applicationName, m_Info.Name.data(), std::min(128LU, m_Info.Name.size()));

    const XrInstanceCreateInfo instance_create_info
    {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .applicationInfo = application_info,
        .enabledExtensionCount = XR_INSTANCE_EXTENSIONS.size(),
        .enabledExtensionNames = XR_INSTANCE_EXTENSIONS.data(),
    };

    return xr::Instance::create(instance_create_info) >> m_XrInstance;
}
