#include <titan/core.hxx>

toolkit::result<> titan::Application::InitializeAudio()
{
    if (auto res = al::Device::Open() >> m_AlDevice; !res)
        return res;
    if (auto res = al::Context::Create(m_AlDevice) >> m_AlContext; !res)
        return res;

    m_AlContext.MakeCurrent();
    return {};
}
