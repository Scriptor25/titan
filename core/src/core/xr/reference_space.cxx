#include <titan/core.hxx>

core::result<> core::Application::CreateReferenceSpace()
{
    return ok()
           & [&]
           {
               const XrReferenceSpaceCreateInfo create_info
               {
                   .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                   .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
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

               return xr::ReferenceSpace::create(m_Session, create_info) >> m_ViewSpace;
           }
           & [&]
           {
               const XrReferenceSpaceCreateInfo create_info
               {
                   .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                   .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
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

               return xr::ReferenceSpace::create(m_Session, create_info) >> m_ReferenceSpace;
           };
}
