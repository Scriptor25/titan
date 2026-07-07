#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::GetViewConfigurationViews()
{
    std::vector<XrViewConfigurationView> views;
    HANDLE(xr::EnumerateViewConfigurationViews(m_XrInstance, m_SystemId, m_ViewConfigurationType) >> views);

    m_SwapchainViews.resize(views.size());
    for (uint32_t i = 0; i < views.size(); ++i)
        m_SwapchainViews[i].View = std::move(views[i]);

    return {};
}
