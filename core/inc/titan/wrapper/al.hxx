#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>

namespace core::al
{
    class Device
    {
    public:
        static result<Device> Open(const ALCchar *device_name = {});

        Device() = default;
        ~Device();

        Device(const Device &) = delete;
        Device &operator=(const Device &) = delete;

        Device(Device &&other) noexcept;
        Device &operator=(Device &&other) noexcept;

        ALCdevice *operator*() const;

    protected:
        explicit Device(ALCdevice *handle);

    private:
        ALCdevice *m_Handle{};
    };

    class Context
    {
    public:
        static result<Context> Create(const Device &device, const ALCint *attr_list = {});

        Context() = default;
        ~Context();

        Context(const Context &) = delete;
        Context &operator=(const Context &) = delete;

        Context(Context &&other) noexcept;
        Context &operator=(Context &&other) noexcept;

        ALCcontext *operator*() const;

        void MakeCurrent() const;

    protected:
        explicit Context(ALCcontext *handle);

    private:
        ALCcontext *m_Handle{};
    };
}
