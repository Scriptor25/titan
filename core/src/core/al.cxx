#include <titan/core.hxx>

core::result<> core::Instance::InitializeAudio()
{
    TRY(al::Device::Open() >> m_AlDevice);
    TRY(al::Context::Create(m_AlDevice) >> m_AlContext);

    m_AlContext.MakeCurrent();

    return ok();
}
