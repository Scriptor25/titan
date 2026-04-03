#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<> core::Application::GetViewConfigurationType()
{
    return xr::EnumerateViewConfigurationTypes(m_XrInstance, m_SystemId)
           & [&](std::vector<XrViewConfigurationType> &&values)
           {
               m_ViewConfigurationType = {};

               for (const auto type : values)
                   for (const auto allowed_type : XR_VIEW_CONFIGURATION_TYPES)
                       if (type == allowed_type)
                       {
                           m_ViewConfigurationType = type;
                           break;
                       }

               if (!m_ViewConfigurationType)
                   return error("failed to find any suitable view configuration type.");

               return ok();
           };
}
