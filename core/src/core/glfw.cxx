#include <titan/core.hxx>

core::result<> core::Instance::InitializeWindow()
{
    TRY(glfw::Library::Create() >> m_GlfwLibrary);

    glfwSetErrorCallback(GlfwDebugCallback);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfw::Monitor monitor;
    TRY(glfw::Monitor::GetPrimary() >> monitor);

    const auto mode = monitor.GetVideoMode();

    TRY(glfw::Window::Create(mode->width, mode->height, "Titan Game", monitor) >> m_GlfwWindow);

    m_GlfwWindow.SetUserPointer(this);
    m_GlfwWindow.Show();

    return ok();
}
