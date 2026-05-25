#include <titan/component.hxx>
#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

titan::Application::Application(ApplicationInfo info)
    : m_Info(std::move(info)),
      m_Inputs(m_XrInstance, m_Session)
{
}

toolkit::result<> titan::Application::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    if (auto res = OnInitialize(); !res)
        return res;

    if (auto res = InitializeWindow(); !res)
        return res;
    if (auto res = InitializeAudio(); !res)
        return res;
    if (auto res = InitializeGraphics(); !res)
        return res;

    if (auto res = m_Inputs.Initialize(); !res)
        return res;

    return OnStart();
}

void titan::Application::Terminate() const
{
    if (m_Window)
        m_Window.Close();
}

toolkit::result<bool> titan::Application::Spin()
{
    glfw::PollEvents();

    if (auto res = PollEvents(); !res || !*res)
        return res;
    if (auto res = RenderFrame(); !res)
        return res;

    return !m_Window.ShouldClose();
}

toolkit::result<> titan::Application::CleanUp()
{
    if (auto res = OnStop(); !res)
        return res;

    if (!m_Device)
        return {};

    if (auto res = vkDeviceWaitIdle(m_Device))
        return toolkit::make_error("vkDeviceWaitIdle => {}", res);

    if (auto res = StorePipelineCache(); !res)
        return res;

    return {};
}

titan::ResourceSystem &titan::Application::GetResources()
{
    return m_Resources;
}

titan::EntitySystem &titan::Application::GetEntities()
{
    return m_Entities;
}

titan::InputSystem &titan::Application::GetInputs()
{
    return m_Inputs;
}

titan::GraphicsSystem &titan::Application::GetGraphics()
{
    return m_Graphics;
}

toolkit::result<> titan::Application::InitializeGraphics()
{
    return toolkit::result()
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
           & WRAP(CreatePipelineLayout)
           & WRAP(CreatePipeline)
           & WRAP(CreateCommandPools)
           & WRAP(AllocateCommandBuffers)
           & WRAP(CreateSynchronization)
           & WRAP(CreateBuffers)
           & WRAP(FillBuffers);
}

toolkit::result<bool> titan::Application::PollEvents()
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

            if (auto res = m_Inputs.RecordBindings(); !res)
                return res;

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
                    return toolkit::make_error("xrBeginSession => {}", res);

                break;
            }

            case XR_SESSION_STATE_STOPPING:
            {
                if (auto res = xrEndSession(m_Session))
                    return toolkit::make_error("xrEndSession => {}", res);

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

toolkit::result<> titan::Application::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    XrFrameState frame_state;
    if (auto res = xr::WaitFrame(m_Session, frame_wait_info) >> frame_state; !res)
        return res;

    if (auto res = PreFrame(); !res)
        return res;

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    if (auto res = xr::BeginFrame(m_Session, frame_begin_info); !res)
        return res;

    if (auto res = OnFrame(); !res)
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
        if (auto res = UpdateComponents(); !res)
            return res;

        if (auto res = m_Inputs.Update(
            {
                .ViewSpace = m_ViewSpace,
                .ReferenceSpace = m_ReferenceSpace,
                .SessionState = m_SessionState,
                .Time = frame_state.predictedDisplayTime,
            }); !res)
            return res;

        if (auto res = RenderThirdEye(frame_state.predictedDisplayTime); !res)
            return res;

        if (auto res = RenderLayer(layer_info); !res)
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

    if (auto res = xr::EndFrame(m_Session, frame_end_info); !res)
        return res;

    return PostFrame();
}

toolkit::result<> titan::Application::RenderLayer(LayerInfo &reference)
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
    if (auto res = xr::LocateViews(m_Session, view_locate_info, view_state) >> views; !res)
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
        return toolkit::make_error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return toolkit::make_error("vkResetFences => {}", res);

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
            return toolkit::make_error("xrAcquireSwapchainImage => {}", res);

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &wait_info))
            return toolkit::make_error("xrWaitSwapchainImage => {}", res);

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

        glm::mat4 view_matrix;
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

            view_matrix = rotation * translation;
        }

        glm::mat4 projection_matrix;
        {
            auto l = NEAR * tanf(view.fov.angleLeft);
            auto r = NEAR * tanf(view.fov.angleRight);
            auto b = NEAR * tanf(view.fov.angleDown);
            auto t = NEAR * tanf(view.fov.angleUp);

            projection_matrix = glm::frustumRH_ZO(l, r, t, b, NEAR, FAR);
        }

        auto screen_matrix = projection_matrix * view_matrix;

        if (auto res = RecordCommandBuffer(width, height, screen_matrix, buffer, framebuffers[image_index]); !res)
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
        return toolkit::make_error("vkQueueSubmit2 => {}", res);

    for (auto &view : m_SwapchainViews)
        if (auto res = xrReleaseSwapchainImage(view.Color.Swapchain, &release_info))
            return toolkit::make_error("xrReleaseSwapchainImage => {}", res);

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(projection_views.size()),
        .views = projection_views.data(),
    };

    return {};
}

toolkit::result<> titan::Application::UpdateComponents()
{
    for (auto [state, transform] : m_Entities.Query<component::Transform>())
    {
        if (!state.Active || !transform.Dirty)
            continue;

        transform.Dirty = false;

        const auto translation = glm::translate(glm::mat4(1.0f), transform.Translation);
        const auto rotation = glm::mat4_cast(transform.Rotation);
        const auto scale = glm::scale(glm::mat4(1.0f), transform.Scale);
        const auto translation_local = glm::translate(glm::mat4(1.0f), -transform.Pivot);

        transform.Matrix = translation * rotation * scale * translation_local;
        transform.Inverse = glm::inverse(transform.Matrix);
    }

    for (auto [state, camera] : m_Entities.Query<component::FrustumCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        camera.Matrix = glm::frustumRH_ZO(
            camera.Left,
            camera.Right,
            camera.Bottom,
            camera.Top,
            camera.Near,
            camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    for (auto [state, camera] : m_Entities.Query<component::AngleFrustumCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        const auto left = camera.Near * tanf(camera.FovLeft);
        const auto right = camera.Near * tanf(camera.FovRight);
        const auto bottom = camera.Near * tanf(camera.FovDown);
        const auto top = camera.Near * tanf(camera.FovUp);

        camera.Matrix = glm::frustumRH_ZO(left, right, bottom, top, camera.Near, camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    int width, height;
    m_Window.GetFramebufferSize(width, height);

    const auto aspect = static_cast<float>(width) / static_cast<float>(height);

    for (auto [state, camera] : m_Entities.Query<component::PerspectiveCamera>())
    {
        if (!state.Active || !camera.Dirty)
            continue;

        camera.Dirty = true;

        camera.Matrix = glm::perspectiveRH_ZO(camera.FovY, aspect, camera.Near, camera.Far);
        camera.Inverse = glm::inverse(camera.Matrix);
    }

    for (auto [state, script] : m_Entities.Query<component::Script>())
    {
        if (!script.Callee)
            continue;

        script.Callee(*this, { state.ID, state.Active });
    }

    return {};
}

toolkit::result<> titan::Application::RenderThirdEye(const XrTime time)
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
        return toolkit::make_error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return toolkit::make_error("vkResetFences => {}", res);

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
        return toolkit::make_error("vkAcquireNextImage2KHR => {}", res);

    int width, height;
    m_Window.GetFramebufferSize(width, height);

    glm::mat4 view_matrix;
    {
        auto [orientation, position] = m_Inputs.GetHead();

        const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
        const auto translation = glm::translate(glm::mat4(1.0f), -position);

        view_matrix = rotation * translation;
    }

    glm::mat4 projection_matrix;
    {
        projection_matrix = glm::perspectiveFovRH_ZO(
            glm::radians(FOV),
            static_cast<float>(width),
            static_cast<float>(height),
            NEAR,
            FAR);

        projection_matrix[1][1] *= -1.0f;
    }

    auto screen_matrix = projection_matrix * view_matrix;

    if (auto res = RecordCommandBuffer(width, height, screen_matrix, buffer, framebuffer); !res)
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
            return toolkit::make_error("vkQueueSubmit2 => {}", res);
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
            return toolkit::make_error("vkQueuePresentKHR =>{}", res);
    }

    return {};
}

toolkit::result<> titan::Application::OnInitialize()
{
    return {};
}

toolkit::result<> titan::Application::OnStart()
{
    return {};
}

toolkit::result<> titan::Application::PreFrame()
{
    return {};
}

toolkit::result<> titan::Application::OnFrame()
{
    return {};
}

toolkit::result<> titan::Application::PostFrame()
{
    return {};
}

toolkit::result<> titan::Application::OnStop()
{
    return {};
}
