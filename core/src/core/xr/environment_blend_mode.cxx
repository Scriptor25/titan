#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

titan::result<> titan::Application::GetEnvironmentBlendMode()
{
    return xr::EnumerateEnvironmentBlendModes(m_XrInstance, m_SystemId, m_ViewConfigurationType)
           & [&](std::vector<XrEnvironmentBlendMode> &&value)
           {
               m_EnvironmentBlendMode = {};

               for (const auto mode : value)
                   for (const auto allowed_mode : XR_ENVIRONMENT_BLEND_MODES)
                       if (mode == allowed_mode)
                       {
                           m_EnvironmentBlendMode = mode;
                           break;
                       }

               if (!m_EnvironmentBlendMode)
               {
                   info("failed to find any suitable environment blend mode.");
                   m_EnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
               }

               return ok();
           };
}
