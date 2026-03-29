#include <titan/wrapper/glfw.hxx>

unsigned core::glfw::Library::instance_count = 0;

core::glfw::Library::Library(const bool initialized)
    : m_Initialized(initialized)
{
    ++instance_count;
}

core::result<core::glfw::Library> core::glfw::Library::Create()
{
    if (!instance_count)
        if (!glfwInit())
            return error<Library>("glfwInit => false");

    return Library(true);
}

core::glfw::Library::~Library()
{
    if (m_Initialized)
    {
        --instance_count;
        if (!instance_count)
            glfwTerminate();
    }
}

core::glfw::Library::Library(Library &&other) noexcept
{
    std::swap(m_Initialized, other.m_Initialized);
}

core::glfw::Library &core::glfw::Library::operator=(Library &&other) noexcept
{
    std::swap(m_Initialized, other.m_Initialized);
    return *this;
}

void core::glfw::PollEvents()
{
    glfwPollEvents();
}

core::glfw::Window::Window(GLFWwindow *handle)
    : m_Handle(handle)
{
}

core::glfw::Window::~Window()
{
    if (m_Handle)
    {
        glfwDestroyWindow(m_Handle);
        m_Handle = {};
    }
}

core::glfw::Window::Window(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

core::glfw::Window &core::glfw::Window::operator=(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

core::glfw::Window::operator GLFWwindow *() const
{
    return m_Handle;
}

void core::glfw::Window::SetUserPointer(void *pointer) const
{
    glfwSetWindowUserPointer(m_Handle, pointer);
}

void core::glfw::Window::Show() const
{
    glfwShowWindow(m_Handle);
}

bool core::glfw::Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Handle);
}

void core::glfw::Window::Close() const
{
    glfwSetWindowShouldClose(m_Handle, GLFW_TRUE);
}

core::glfw::Monitor::Monitor(GLFWmonitor *handle)
    : m_Handle(handle)
{
}

core::result<core::glfw::Window> core::glfw::Window::Create(
    const int width,
    const int height,
    const char *title,
    const Monitor &monitor,
    const Window &share)
{
    if (const auto window = glfwCreateWindow(width, height, title, monitor, share))
        return Window(window);
    return error<Window>("glfwCreateWindow => null");
}

core::result<core::glfw::Window> core::glfw::Window::Create(
    const int width,
    const int height,
    const char *title,
    const Monitor &monitor)
{
    return Create(width, height, title, monitor, {});
}

core::result<core::glfw::Window> core::glfw::Window::Create(
    const int width,
    const int height,
    const char *title)
{
    return Create(width, height, title, {}, {});
}

core::result<core::glfw::Monitor> core::glfw::Monitor::GetPrimary()
{
    if (const auto monitor = glfwGetPrimaryMonitor())
        return Monitor(monitor);
    return error<Monitor>("glfwGetPrimaryMonitor => null");
}

core::glfw::Monitor::~Monitor()
{
    m_Handle = {};
}

core::glfw::Monitor::Monitor(Monitor &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

core::glfw::Monitor &core::glfw::Monitor::operator=(Monitor &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

core::glfw::Monitor::operator GLFWmonitor *() const
{
    return m_Handle;
}

const GLFWvidmode *core::glfw::Monitor::GetVideoMode() const
{
    return glfwGetVideoMode(m_Handle);
}
