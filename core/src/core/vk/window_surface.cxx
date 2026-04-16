#include <titan/core.hxx>

titan::result<> titan::Application::CreateWindowSurface()
{
    return vk::SurfaceKHR::create(m_VkInstance, m_Window) >> m_WindowSurface;
}
