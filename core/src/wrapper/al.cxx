#include <titan/wrapper/al.hxx>

toolkit::result<titan::al::Device> titan::al::Device::Open(const ALCchar *device_name)
{
    if (const auto device = alcOpenDevice(device_name))
        return Device(device);
    return toolkit::make_error("alcOpenDevice => null");
}

titan::al::Device::~Device()
{
    if (m_Handle)
    {
        alcCloseDevice(m_Handle);
        m_Handle = {};
    }
}

titan::al::Device::Device(Device &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

titan::al::Device &titan::al::Device::operator=(Device &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

ALCdevice *titan::al::Device::operator*() const
{
    return m_Handle;
}

titan::al::Device::Device(ALCdevice *handle)
    : m_Handle(handle)
{
}

toolkit::result<titan::al::Context> titan::al::Context::Create(const Device &device, const ALCint *attr_list)
{
    if (const auto context = alcCreateContext(*device, attr_list))
        return Context(context);
    return toolkit::make_error("alcCreateContext => null");
}

titan::al::Context::~Context()
{
    if (m_Handle)
    {
        alcDestroyContext(m_Handle);
        m_Handle = {};
    }
}

titan::al::Context::Context(Context &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

titan::al::Context &titan::al::Context::operator=(Context &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

ALCcontext *titan::al::Context::operator*() const
{
    return m_Handle;
}

void titan::al::Context::MakeCurrent() const
{
    alcMakeContextCurrent(m_Handle);
}

titan::al::Context::Context(ALCcontext *handle)
    : m_Handle(handle)
{
}
