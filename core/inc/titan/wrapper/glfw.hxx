#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>

namespace titan::glfw
{
    class Window;
    class Monitor;

    class Monitor
    {
    public:
        static result<Monitor> GetPrimary();

        Monitor() = default;
        ~Monitor();

        Monitor(const Monitor &) = delete;
        Monitor &operator=(const Monitor &) = delete;

        Monitor(Monitor &&other) noexcept;
        Monitor &operator=(Monitor &&other) noexcept;

        operator GLFWmonitor *() const;

        [[nodiscard]] const GLFWvidmode *GetVideoMode() const;

    protected:
        explicit Monitor(GLFWmonitor *handle);

    private:
        GLFWmonitor *m_Handle{};
    };

    class Window
    {
    public:
        static result<Window> Create(
            int width,
            int height,
            const char *title,
            const Monitor &monitor,
            const Window &share);

        static result<Window> Create(
            int width,
            int height,
            const char *title,
            const Monitor &monitor);

        static result<Window> Create(
            int width,
            int height,
            const char *title);

        Window() = default;
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        Window(Window &&other) noexcept;
        Window &operator=(Window &&other) noexcept;

        operator GLFWwindow *() const;

        void SetUserPointer(void *pointer) const;
        void Show() const;

        [[nodiscard]] bool ShouldClose() const;
        void Close() const;

        void GetFramebufferSize(int &width, int &height) const;

    protected:
        explicit Window(GLFWwindow *handle);

    private:
        GLFWwindow *m_Handle{};
    };

    class Instance
    {
        static unsigned count;

        explicit Instance(bool initialized);

    public:
        static result<Instance> Create();

        Instance() = default;
        ~Instance();

        Instance(const Instance &) = delete;
        Instance &operator=(const Instance &) = delete;

        Instance(Instance &&other) noexcept;
        Instance &operator=(Instance &&other) noexcept;

    private:
        bool m_Initialized{};
    };

    void PollEvents();
}
