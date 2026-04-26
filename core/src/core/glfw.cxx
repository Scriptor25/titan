#include <titan/core.hxx>

toolkit::result<> titan::Application::InitializeWindow()
{
    return ok()
           & [&]
           {
               return glfw::Instance::Create() >> m_GlfwInstance;
           }
           & []
           {
               glfwSetErrorCallback(GlfwDebugCallback);

               glfwDefaultWindowHints();
               glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
               glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
               glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

               return glfw::Monitor::GetPrimary();
           }
           & [&](glfw::Monitor &&monitor)
           {
               const auto mode = monitor.GetVideoMode();

               return glfw::Window::Create(mode->width, mode->height, m_Info.Name.c_str(), monitor) >> m_Window;
           }
           & [&]
           {
               m_Window.SetUserPointer(this);
               m_Window.Show();

               return ok();
           };
}
