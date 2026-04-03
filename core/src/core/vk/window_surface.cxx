#include <titan/core.hxx>

core::result<> core::Application::CreateWindowSurface()
{
    return vk::SurfaceKHR::create(m_VkInstance, m_Window) >> m_WindowSurface;
}
