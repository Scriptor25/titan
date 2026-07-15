#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::InitializeXrEnvironmentBlendMode()
{
    return xr::EnumerateEnvironmentBlendModes(m_XrInstance, m_XrSystemId, m_XrViewConfigurationType)
           & [&](std::vector<XrEnvironmentBlendMode> &&value) -> toolkit::result<>
           {
               m_XrEnvironmentBlendMode = {};

               for (const auto mode : value)
                   for (const auto allowed_mode : XR_ENVIRONMENT_BLEND_MODES)
                       if (mode == allowed_mode)
                       {
                           m_XrEnvironmentBlendMode = mode;
                           break;
                       }

               if (!m_XrEnvironmentBlendMode)
               {
                   info("failed to find any suitable environment blend mode.");
                   m_XrEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
               }

               return {};
           };
}
