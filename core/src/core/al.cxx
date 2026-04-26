#include <titan/core.hxx>

toolkit::result<> titan::Application::InitializeAudio()
{
    return ok()
           & [&]
           {
               return al::Device::Open() >> m_AlDevice;
           }
           & [&]
           {
               return al::Context::Create(m_AlDevice) >> m_AlContext;
           }
           & [&]
           {
               m_AlContext.MakeCurrent();
               return ok();
           };
}
