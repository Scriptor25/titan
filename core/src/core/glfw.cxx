#include <titan/core.hxx>

core::result<> core::Application::InitializeWindow()
{
    TRY(glfw::Instance::Create() >> m_GlfwInstance);

    glfwSetErrorCallback(GlfwDebugCallback);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfw::Monitor monitor;
    TRY(glfw::Monitor::GetPrimary() >> monitor);

    const auto mode = monitor.GetVideoMode();

    TRY(glfw::Window::Create(mode->width, mode->height, m_Info.Name.c_str(), monitor) >> m_Window);

    m_Window.SetUserPointer(this);
    m_Window.Show();

    return ok();
}
