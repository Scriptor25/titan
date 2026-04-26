#include <titan/core.hxx>

toolkit::result<> titan::Application::CreateWindowSurface()
{
    return vk::SurfaceKHR::create(m_VkInstance, m_Window) >> m_WindowSurface;
}
