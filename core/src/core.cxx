#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

core::Application::Application(ApplicationInfo info)
    : m_Info(std::move(info))
{
}

core::result<> core::Application::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    m_Mesh = obj::Open("res/mesh/teapot.obj");

    if (auto res = InitializeWindow())
        return res;
    if (auto res = InitializeAudio())
        return res;
    if (auto res = InitializeGraphics())
        return res;

    return OnStart();
}

void core::Application::Terminate() const
{
    if (m_Window)
        m_Window.Close();
}

core::result<bool> core::Application::Spin()
{
    glfw::PollEvents();

    if (auto res = PollEvents(); res || !*res)
        return res;
    if (auto res = RenderFrame())
        return res;

    return !m_Window.ShouldClose();
}

core::result<> core::Application::CleanUp()
{
    if (auto res = OnStop())
        return res;

    if (auto res = vkDeviceWaitIdle(m_Device))
        return error("vkDeviceWaitIdle => {}", res);

    if (auto res = StorePipelineCache())
        return res;

    return ok();
}

core::result<> core::Application::InitializeGraphics()
{
    return ok()
           & WRAP(CreateXrInstance)
           & WRAP(CreateXrMessenger)
           & WRAP(GetSystemId)
           & WRAP(CreateVkInstance)
           & WRAP(CreateVkMessenger)
           & WRAP(GetPhysicalDevice)
           & WRAP(GetFormats)
           & WRAP(CreateWindowSurface)
           & WRAP(GetQueueFamilyIndices)
           & WRAP(CreateDevice)
           & WRAP(CreateWindowSwapchainView)
           & WRAP(GetDeviceQueues)
           & WRAP(CreateSession)
           & WRAP(GetViewConfigurationType)
           & WRAP(GetViewConfigurationViews)
           & WRAP(CreateSwapchainViews)
           & WRAP(GetEnvironmentBlendMode)
           & WRAP(CreateReferenceSpace)
           & WRAP(CreateRenderPass)
           & WRAP(CreateFramebuffers)
           & WRAP(CreatePipelineCache)
           & WRAP(CreateDescriptorPool)
           & WRAP(CreateDescriptorSetLayouts)
           & WRAP(AllocateDescriptorSets)
           & WRAP(CreatePipelineLayout)
           & WRAP(CreatePipeline)
           & WRAP(CreateCommandPools)
           & WRAP(AllocateCommandBuffers)
           & WRAP(CreateSynchronization)
           & WRAP(CreateBuffers)
           & WRAP(AllocateBufferMemory)
           & WRAP(FillVertexBuffer);
}

core::result<bool> core::Application::PollEvents()
{
    XrEventDataBuffer event_data{ .type = XR_TYPE_EVENT_DATA_BUFFER };

    while (xrPollEvent(m_XrInstance, &event_data) == XR_SUCCESS)
    {
        switch (event_data.type)
        {
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
        {
            const auto events_lost = reinterpret_cast<XrEventDataEventsLost *>(&event_data);
            info("XrEventDataEventsLost {{ lostEventCount={} }}", events_lost->lostEventCount);
            break;
        }

        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
            const auto instance_loss_pending = reinterpret_cast<XrEventDataInstanceLossPending *>(&event_data);
            info("XrEventDataInstanceLossPending {{ lossTime={} }}", instance_loss_pending->lossTime);

            return false;
        }

        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
            const auto interaction_profile_changed = reinterpret_cast<XrEventDataInteractionProfileChanged *>(
                &event_data);
            info(
                "XrEventDataInteractionProfileChanged {{ session={} }}",
                static_cast<void *>(interaction_profile_changed->session));

            if (interaction_profile_changed->session != m_Session)
            {
                info("XrEventDataInteractionProfileChanged for foreign session!");
                break;
            }

            break;
        }

        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
        {
            const auto reference_space_change_pending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(
                &event_data);
            info(
                "XrEventDataReferenceSpaceChangePending {{ session={}, referenceSpaceType={}, changeTime={}, poseValid={} }}",
                static_cast<void *>(reference_space_change_pending->session),
                reference_space_change_pending->referenceSpaceType,
                reference_space_change_pending->changeTime,
                reference_space_change_pending->poseValid);

            if (reference_space_change_pending->session != m_Session)
            {
                info("XrEventDataReferenceSpaceChangePending for foreign session!");
                break;
            }

            break;
        }

        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            const auto session_state_changed = reinterpret_cast<XrEventDataSessionStateChanged *>(&event_data);
            info(
                "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                static_cast<void *>(session_state_changed->session),
                session_state_changed->state,
                session_state_changed->time);

            if (session_state_changed->session != m_Session)
            {
                info("XrEventDataSessionStateChanged for foreign session!");
                break;
            }

            m_SessionState = session_state_changed->state;

            switch (session_state_changed->state)
            {
            case XR_SESSION_STATE_READY:
            {
                const XrSessionBeginInfo session_begin_info
                {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .primaryViewConfigurationType = m_ViewConfigurationType,
                };

                if (auto res = xrBeginSession(m_Session, &session_begin_info))
                    return error<bool>("xrBeginSession => {}", res);

                break;
            }

            case XR_SESSION_STATE_STOPPING:
            {
                if (auto res = xrEndSession(m_Session))
                    return error<bool>("xrEndSession => {}", res);

                info(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
                return false;
            }

            case XR_SESSION_STATE_LOSS_PENDING:
            case XR_SESSION_STATE_EXITING:
            {
                info(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
                return false;
            }

            default:
                break;
            }

            break;
        }

        default:
            break;
        }

        event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    return true;
}

core::result<> core::Application::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    XrFrameState frame_state;
    if (auto res = xr::WaitFrame(m_Session, frame_wait_info) >> frame_state)
        return res;

    if (auto res = PreFrame())
        return res;

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    if (auto res = xr::BeginFrame(m_Session, frame_begin_info))
        return res;

    if (auto res = OnFrame())
        return res;

    LayerInfo layer_info
    {
        .PredictedDisplayTime = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_SessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_SessionState == XR_SESSION_STATE_VISIBLE
                                || m_SessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        if (auto res = UpdateModel())
            return res;
        if (auto res = RenderThirdEye(frame_state.predictedDisplayTime))
            return res;
        if (auto res = RenderLayer(layer_info))
            return res;

        layer_info.Layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer_info.Projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_EnvironmentBlendMode,
        .layerCount = static_cast<uint32_t>(layer_info.Layers.size()),
        .layers = layer_info.Layers.data(),
    };

    if (auto res = xr::EndFrame(m_Session, frame_end_info))
        return res;

    return PostFrame();
}

core::result<> core::Application::RenderLayer(LayerInfo &reference)
{
    auto &projection_views = reference.Views;

    const XrViewLocateInfo view_locate_info
    {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = m_ViewConfigurationType,
        .displayTime = reference.PredictedDisplayTime,
        .space = m_ReferenceSpace,
    };

    XrViewState view_state
    {
        .type = XR_TYPE_VIEW_STATE,
    };

    std::vector<XrView> views;
    if (auto res = xr::LocateViews(m_Session, view_locate_info, view_state) >> views)
        return res;

    projection_views = {
        views.size(),
        { .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW },
    };

    std::vector<VkCommandBufferSubmitInfo> command_buffers
    {
        views.size(),
        { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO },
    };

    const std::vector<VkFence> fences
    {
        m_Fence,
    };

    if (auto res = vkWaitForFences(
        m_Device,
        fences.size(),
        fences.data(),
        true,
        std::numeric_limits<uint64_t>::max()))
        return error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return error("vkResetFences => {}", res);

    const XrSwapchainImageAcquireInfo acquire_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
    };

    const XrSwapchainImageReleaseInfo release_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    const XrSwapchainImageWaitInfo wait_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
        .timeout = XR_INFINITE_DURATION,
    };

    for (uint32_t view_index = 0; view_index < views.size(); ++view_index)
    {
        const auto &view = views[view_index];

        auto &[
            view_configuration_view,
            color,
            depth,
            framebuffers,
            buffer
        ] = m_SwapchainViews[view_index];

        uint32_t image_index;
        if (auto res = xrAcquireSwapchainImage(color.Swapchain, &acquire_info, &image_index))
            return error("xrAcquireSwapchainImage => {}", res);

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &wait_info))
            return error("xrWaitSwapchainImage => {}", res);

        const auto width = view_configuration_view.recommendedImageRectWidth;
        const auto height = view_configuration_view.recommendedImageRectHeight;

        projection_views[view_index] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .pose = view.pose,
            .fov = view.fov,
            .subImage = {
                .swapchain = color.Swapchain,
                .imageRect = {
                    .offset = {
                        .x = 0,
                        .y = 0,
                    },
                    .extent = {
                        .width = static_cast<int32_t>(width),
                        .height = static_cast<int32_t>(height),
                    },
                },
                .imageArrayIndex = 0,
            },
        };

        command_buffers[view_index] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = buffer,
        };

        glm::mat4 view_mat;
        {
            const glm::quat orientation
            {
                view.pose.orientation.w,
                view.pose.orientation.x,
                view.pose.orientation.y,
                view.pose.orientation.z,
            };

            const glm::vec3 position
            {
                view.pose.position.x,
                view.pose.position.y,
                view.pose.position.z,
            };

            const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
            const auto translation = glm::translate(glm::mat4(1.0f), -position);

            view_mat = rotation * translation;
        }

        glm::mat4 proj_mat;
        {
            auto l = NEAR * tanf(view.fov.angleLeft);
            auto r = NEAR * tanf(view.fov.angleRight);
            auto b = NEAR * tanf(view.fov.angleDown);
            auto t = NEAR * tanf(view.fov.angleUp);

            proj_mat = glm::frustumRH_ZO(l, r, t, b, NEAR, FAR);
        }

        const CameraData camera_data
        {
            .Screen = proj_mat * view_mat,
            .Model = m_Model,
            .Normal = m_Normal,
        };

        if (auto res = RecordCommandBuffer(width, height, camera_data, buffer, framebuffers[image_index]))
            return res;
    }

    const std::array submits
    {
        VkSubmitInfo2
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = static_cast<uint32_t>(command_buffers.size()),
            .pCommandBufferInfos = command_buffers.data(),
        },
    };

    if (auto res = vkQueueSubmit2(m_DefaultQueue, submits.size(), submits.data(), m_Fence))
        return error("vkQueueSubmit2 => {}", res);

    for (auto &view : m_SwapchainViews)
        if (auto res = xrReleaseSwapchainImage(view.Color.Swapchain, &release_info))
            return error("xrReleaseSwapchainImage => {}", res);

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(projection_views.size()),
        .views = projection_views.data(),
    };

    return ok();
}

core::result<> core::Application::UpdateModel()
{
    static auto begin = std::chrono::high_resolution_clock::now();
    const auto now = std::chrono::high_resolution_clock::now();

    const auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - begin).count();

    m_Model = { 1.0f };
    m_Model = glm::translate(m_Model, glm::vec3(0.0f, 0.0f, 0.0f));
    m_Model = glm::scale(m_Model, glm::vec3(0.1f));
    m_Model = glm::rotate(m_Model, delta * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // m_Model = glm::rotate(m_Model, delta * glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 1.0f));
    // m_Model = glm::translate(m_Model, glm::vec3(-0.5f, -0.5f, -0.5f));

    m_Normal = glm::transpose(glm::inverse(m_Model));

    return ok();
}

core::result<> core::Application::RenderThirdEye(const XrTime time)
{
    auto &[
        available,
        finished,
        fence,
        framebuffer,
        buffer
    ] = m_Frames[m_FrameIndex];
    m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();

    const std::vector<VkFence> fences
    {
        fence,
    };

    if (auto res = vkWaitForFences(
        m_Device,
        fences.size(),
        fences.data(),
        true,
        std::numeric_limits<uint64_t>::max()))
        return error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return error("vkResetFences => {}", res);

    const VkAcquireNextImageInfoKHR acquire_info
    {
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .swapchain = m_WindowSwapchainView.Color.Swapchain,
        .timeout = std::numeric_limits<uint64_t>::max(),
        .semaphore = available,
        .fence = nullptr,
        .deviceMask = 1,
    };

    uint32_t image_index;
    if (auto res = vkAcquireNextImage2KHR(m_Device, &acquire_info, &image_index))
        return error("vkAcquireNextImage2KHR => {}", res);

    XrSpaceLocation view
    {
        .type = XR_TYPE_SPACE_LOCATION,
    };

    if (auto res = xrLocateSpace(m_ViewSpace, m_ReferenceSpace, time, &view))
        return error("xrLocateSpace => {}", res);

    int width, height;
    m_Window.GetFramebufferSize(width, height);

    glm::mat4 view_mat;
    {
        glm::quat orientation
        {
            view.pose.orientation.w,
            view.pose.orientation.x,
            view.pose.orientation.y,
            view.pose.orientation.z,
        };

        glm::vec3 position
        {
            view.pose.position.x,
            view.pose.position.y,
            view.pose.position.z,
        };

        orientation = glm::slerp(m_HeadPose.Orientation, orientation, 0.1f);
        position = glm::mix(m_HeadPose.Position, position, 0.1f);

        m_HeadPose = {
            .Orientation = orientation,
            .Position = position,
        };

        const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
        const auto translation = glm::translate(glm::mat4(1.0f), -position);

        view_mat = rotation * translation;
    }

    glm::mat4 proj_mat;
    {
        proj_mat = glm::perspectiveFovRH_ZO(
            glm::radians(FOV),
            static_cast<float>(width),
            static_cast<float>(height),
            NEAR,
            FAR);

        proj_mat[1][1] *= -1.0f;
    }

    const CameraData camera_data
    {
        .Screen = proj_mat * view_mat,
        .Model = m_Model,
        .Normal = m_Normal,
    };

    if (auto res = RecordCommandBuffer(width, height, camera_data, buffer, framebuffer))
        return res;

    {
        const std::array wait_semaphores
        {
            VkSemaphoreSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = available,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            },
        };

        const std::array command_buffers
        {
            VkCommandBufferSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = buffer,
            }
        };

        const std::array signal_semaphores
        {
            VkSemaphoreSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = finished,
                .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            },
        };

        const std::array submits
        {
            VkSubmitInfo2
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = wait_semaphores.size(),
                .pWaitSemaphoreInfos = wait_semaphores.data(),
                .commandBufferInfoCount = command_buffers.size(),
                .pCommandBufferInfos = command_buffers.data(),
                .signalSemaphoreInfoCount = signal_semaphores.size(),
                .pSignalSemaphoreInfos = signal_semaphores.data(),
            },
        };

        if (auto res = vkQueueSubmit2(m_DefaultQueue, submits.size(), submits.data(), fence))
            return error("vkQueueSubmit2 => {}", res);
    }

    {
        const std::vector<VkSemaphore> wait_semaphores
        {
            finished,
        };

        const std::vector<VkSwapchainKHR> swapchains
        {
            m_WindowSwapchainView.Color.Swapchain,
        };

        const VkPresentInfoKHR present_info
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
            .pWaitSemaphores = wait_semaphores.data(),
            .swapchainCount = static_cast<uint32_t>(swapchains.size()),
            .pSwapchains = swapchains.data(),
            .pImageIndices = &image_index,
        };

        if (auto res = vkQueuePresentKHR(m_PresentQueue, &present_info))
            return error("vkQueuePresentKHR =>{}", res);
    }

    return ok();
}

core::result<> core::Application::OnStart()
{
    return ok();
}

core::result<> core::Application::PreFrame()
{
    return ok();
}

core::result<> core::Application::OnFrame()
{
    return ok();
}

core::result<> core::Application::PostFrame()
{
    return ok();
}

core::result<> core::Application::OnStop()
{
    return ok();
}
