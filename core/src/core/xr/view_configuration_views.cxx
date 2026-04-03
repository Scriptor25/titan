#include <titan/core.hxx>

core::result<> core::Application::GetViewConfigurationViews()
{
    return xr::EnumerateViewConfigurationViews(m_XrInstance, m_SystemId, m_ViewConfigurationType)
           | [this](std::vector<XrViewConfigurationView> &&views)
           {
               m_SwapchainViews.resize(views.size());
               for (uint32_t i = 0; i < views.size(); ++i)
                   m_SwapchainViews[i].View = views[i];
               return ok();
           };;
}
