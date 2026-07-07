#include <titan/core.hxx>

toolkit::result<> titan::Application::CreateReferenceSpaces()
{
    HANDLE(CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_VIEW) >> m_ViewSpace);
    HANDLE(CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_STAGE) >> m_ReferenceSpace);

    return {};
}

toolkit::result<titan::xr::ReferenceSpace> titan::Application::CreateReferenceSpace(XrReferenceSpaceType type)
{
    const XrReferenceSpaceCreateInfo create_info
    {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = type,
        .poseInReferenceSpace = {
            .orientation = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
                .w = 1.0f,
            },
            .position = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
            },
        },
    };

    return xr::ReferenceSpace::create(m_Session, create_info);
}
