#include <titan/core.hxx>

toolkit::result<> titan::Application::InitializeAudio()
{
    HANDLE(al::Device::Open() >> m_AlDevice);
    HANDLE(al::Context::Create(m_AlDevice) >> m_AlContext);

    m_AlContext.MakeCurrent();
    return {};
}
