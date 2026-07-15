#include <titan/core.hxx>

toolkit::result<> titan::Application::InitializeWindow()
{
    if (auto res = glfw::Instance::Create() >> m_GlfwInstance; !res)
        return res;

    glfwSetErrorCallback(GlfwDebugCallback);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfw::Monitor monitor;
    if (auto res = glfw::Monitor::GetPrimary() >> monitor; !res)
        return res;

    const auto mode = monitor.GetVideoMode();

    if (auto res = glfw::Window::Create(mode->width, mode->height, m_Info.Name.c_str(), monitor) >> m_Window; !res)
        return res;

    m_Window.SetUserPointer(this);
    m_Window.Show();

    return {};
}
