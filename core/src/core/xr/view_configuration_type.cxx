#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::GetViewConfigurationType()
{
    std::vector<XrViewConfigurationType> values;
    HANDLE(xr::EnumerateViewConfigurationTypes(m_XrInstance, m_SystemId) >>values);

    m_ViewConfigurationType = {};

    for (const auto type : values)
        for (const auto allowed_type : XR_VIEW_CONFIGURATION_TYPES)
            if (type == allowed_type)
            {
                m_ViewConfigurationType = type;
                break;
            }

    if (!m_ViewConfigurationType)
        return toolkit::make_error("failed to find any suitable view configuration type.");

    return {};
}
