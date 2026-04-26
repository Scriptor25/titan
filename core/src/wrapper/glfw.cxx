#include <titan/wrapper/glfw.hxx>

unsigned titan::glfw::Instance::count = 0;

titan::glfw::Instance::Instance(const bool initialized)
    : m_Initialized(initialized)
{
    ++count;
}

toolkit::result<titan::glfw::Instance> titan::glfw::Instance::Create()
{
    if (!count)
        if (!glfwInit())
            return toolkit::make_error("glfwInit => false");

    return Instance(true);
}

titan::glfw::Instance::~Instance()
{
    if (m_Initialized)
    {
        --count;
        if (!count)
            glfwTerminate();
    }
}

titan::glfw::Instance::Instance(Instance &&other) noexcept
{
    std::swap(m_Initialized, other.m_Initialized);
}

titan::glfw::Instance &titan::glfw::Instance::operator=(Instance &&other) noexcept
{
    std::swap(m_Initialized, other.m_Initialized);
    return *this;
}

void titan::glfw::PollEvents()
{
    glfwPollEvents();
}

titan::glfw::Window::Window(GLFWwindow *handle)
    : m_Handle(handle)
{
}

titan::glfw::Window::~Window()
{
    if (m_Handle)
    {
        glfwDestroyWindow(m_Handle);
        m_Handle = {};
    }
}

titan::glfw::Window::Window(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

titan::glfw::Window &titan::glfw::Window::operator=(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

titan::glfw::Window::operator GLFWwindow *() const
{
    return m_Handle;
}

void titan::glfw::Window::SetUserPointer(void *pointer) const
{
    glfwSetWindowUserPointer(m_Handle, pointer);
}

void titan::glfw::Window::Show() const
{
    glfwShowWindow(m_Handle);
}

bool titan::glfw::Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Handle);
}

void titan::glfw::Window::Close() const
{
    glfwSetWindowShouldClose(m_Handle, GLFW_TRUE);
}

void titan::glfw::Window::GetFramebufferSize(int &width, int &height) const
{
    glfwGetFramebufferSize(m_Handle, &width, &height);
}

titan::glfw::Monitor::Monitor(GLFWmonitor *handle)
    : m_Handle(handle)
{
}

toolkit::result<titan::glfw::Window> titan::glfw::Window::Create(
    const int width,
    const int height,
    const char *title,
    const Monitor &monitor,
    const Window &share)
{
    if (const auto window = glfwCreateWindow(width, height, title, monitor, share))
        return Window(window);
    return toolkit::make_error("glfwCreateWindow => null");
}

toolkit::result<titan::glfw::Window> titan::glfw::Window::Create(
    const int width,
    const int height,
    const char *title,
    const Monitor &monitor)
{
    return Create(width, height, title, monitor, {});
}

toolkit::result<titan::glfw::Window> titan::glfw::Window::Create(
    const int width,
    const int height,
    const char *title)
{
    return Create(width, height, title, {}, {});
}

toolkit::result<titan::glfw::Monitor> titan::glfw::Monitor::GetPrimary()
{
    if (const auto monitor = glfwGetPrimaryMonitor())
        return Monitor(monitor);
    return toolkit::make_error("glfwGetPrimaryMonitor => null");
}

titan::glfw::Monitor::~Monitor()
{
    m_Handle = {};
}

titan::glfw::Monitor::Monitor(Monitor &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

titan::glfw::Monitor &titan::glfw::Monitor::operator=(Monitor &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

titan::glfw::Monitor::operator GLFWmonitor *() const
{
    return m_Handle;
}

const GLFWvidmode *titan::glfw::Monitor::GetVideoMode() const
{
    return glfwGetVideoMode(m_Handle);
}
