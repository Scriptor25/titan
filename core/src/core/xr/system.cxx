#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::GetSystemId()
{
    const XrSystemGetInfo get_info
    {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    return xr::GetSystem(m_XrInstance, get_info) >> m_SystemId;
}
