#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::InitializeXrViewConfigurationType()
{
    return xr::EnumerateViewConfigurationTypes(m_XrInstance, m_XrSystemId)
           & [&](std::vector<XrViewConfigurationType> &&values) -> toolkit::result<>
           {
               m_XrViewConfigurationType = {};

               for (const auto type : values)
                   for (const auto allowed_type : XR_VIEW_CONFIGURATION_TYPES)
                       if (type == allowed_type)
                       {
                           m_XrViewConfigurationType = type;
                           break;
                       }

               if (!m_XrViewConfigurationType)
                   return toolkit::make_error("failed to find any suitable view configuration type.");

               return {};
           };
}

toolkit::result<> titan::Application::InitializeXrViewConfigurationViews()
{
    return xr::EnumerateViewConfigurationViews(m_XrInstance, m_XrSystemId, m_XrViewConfigurationType)
           >> m_XrViewConfigurationViews;
}
