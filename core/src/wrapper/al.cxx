#include <result.hxx>
#include <wrapper/al.hxx>

core::result<core::al::Device> core::al::Device::Open(const ALCchar *device_name)
{
    if (const auto device = alcOpenDevice(device_name))
        return Device(device);
    return error<Device>("alcOpenDevice => null");
}

core::al::Device::~Device()
{
    if (m_Handle)
    {
        alcCloseDevice(m_Handle);
        m_Handle = {};
    }
}

core::al::Device::Device(Device &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

core::al::Device &core::al::Device::operator=(Device &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

ALCdevice *core::al::Device::operator*() const
{
    return m_Handle;
}

core::al::Device::Device(ALCdevice *handle)
    : m_Handle(handle)
{
}

core::result<core::al::Context> core::al::Context::Create(const Device &device, const ALCint *attr_list)
{
    if (const auto context = alcCreateContext(*device, attr_list))
        return Context(context);
    return error<Context>("alcCreateContext => null");
}

core::al::Context::~Context()
{
    if (m_Handle)
    {
        alcDestroyContext(m_Handle);
        m_Handle = {};
    }
}

core::al::Context::Context(Context &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

core::al::Context &core::al::Context::operator=(Context &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

ALCcontext *core::al::Context::operator*() const
{
    return m_Handle;
}

void core::al::Context::MakeCurrent() const
{
    alcMakeContextCurrent(m_Handle);
}

core::al::Context::Context(ALCcontext *handle)
    : m_Handle(handle)
{
}
