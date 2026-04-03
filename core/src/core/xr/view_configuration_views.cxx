#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<> core::Application::GetViewConfigurationViews()
{
    return xr::EnumerateViewConfigurationViews(m_XrInstance, m_SystemId, m_ViewConfigurationType)
           & [&](std::vector<XrViewConfigurationView> &&views)
           {
               m_SwapchainViews.resize(views.size());
               for (uint32_t i = 0; i < views.size(); ++i)
                   m_SwapchainViews[i].View = std::move(views[i]);
               return ok();
           };;
}
