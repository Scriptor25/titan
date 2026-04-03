#include <titan/core.hxx>

core::result<> core::Application::GetSystemId()
{
    const XrSystemGetInfo system_get_info
    {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    if (auto res = xrGetSystem(m_XrInstance, &system_get_info, &m_SystemId))
        return error("xrGetSystem => {}", res);

    return ok();
}
