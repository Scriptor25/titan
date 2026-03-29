#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/format/xr.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstring>

core::result<> core::Instance::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    m_Mesh = obj::Open("res/mesh/teapot.obj");

    TRY(InitializeWindow());
    TRY(InitializeAudio());
    TRY(InitializeGraphics());

    return ok();
}

void core::Instance::Terminate()
{
    m_GlfwWindow.Close();
}

core::result<> core::Instance::Spin()
{
    glfw::PollEvents();

    if (m_GlfwWindow.ShouldClose())
        return error("window should close");

    TRY(PollEvents());
    TRY(RenderFrame());

    return ok();
}

core::result<> core::Instance::InitializeGraphics()
{
    TRY(CreateXrInstance());
    TRY(CreateXrMessenger());
    TRY(GetSystemId());
    TRY(CreateVkInstance());
    TRY(CreateVkMessenger());
    TRY(GetPhysicalDevice());
    TRY(CreateDevice());
    TRY(GetQueueFamilyIndex());
    TRY(GetQueueIndex());
    TRY(CreateSession());
    TRY(GetViewConfigurationType());
    TRY(GetViewConfigurationViews());
    TRY(GetFormats());
    TRY(CreateSwapchains());
    TRY(GetEnvironmentBlendMode());
    TRY(CreateReferenceSpace());
    TRY(CreateSurface());
    TRY(CreateRenderPass());
    TRY(CreateFramebuffers());
    TRY(CreatePipelineCache());
    TRY(CreateDescriptorSetLayout());
    TRY(CreateDescriptorPool());
    TRY(CreateDescriptorSet());
    TRY(CreatePipelineLayout());
    TRY(CreatePipeline());
    TRY(CreateCommandPool());
    TRY(CreateCommandBuffer());
    TRY(CreateSynchronization());
    TRY(CreateVertexBuffer());
    TRY(CreateVertexMemory());
    TRY(FillVertexBuffer());
    TRY(CreateCameraBuffer());
    TRY(CreateCameraMemory());

    return ok();
}

core::result<> core::Instance::PollEvents()
{
    XrEventDataBuffer event_data
    {
        .type = XR_TYPE_EVENT_DATA_BUFFER,
        .next = nullptr,
        .varying = {},
    };

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

            return error("XrEventDataInstanceLossPending {{ lossTime={} }}", instance_loss_pending->lossTime);
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
                    .next = nullptr,
                    .primaryViewConfigurationType = m_ViewConfigurationType,
                };

                if (auto res = xrBeginSession(m_Session, &session_begin_info))
                    return error("xrBeginSession => {}", res);

                break;
            }

            case XR_SESSION_STATE_STOPPING:
            {
                if (auto res = xrEndSession(m_Session))
                    return error("xrEndSession => {}", res);

                return error(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
            }

            case XR_SESSION_STATE_LOSS_PENDING:
            case XR_SESSION_STATE_EXITING:
            {
                return error(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
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

    return ok();
}

core::result<> core::Instance::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
        .next = nullptr,
    };

    XrFrameState frame_state
    {
        .type = XR_TYPE_FRAME_STATE,
        .next = nullptr,
        .predictedDisplayTime = {},
        .predictedDisplayPeriod = {},
        .shouldRender = {},
    };

    if (auto res = xrWaitFrame(m_Session, &frame_wait_info, &frame_state))
        return error("xrWaitFrame => {}", res);

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
        .next = nullptr,
    };

    if (auto res = xrBeginFrame(m_Session, &frame_begin_info))
        return error("xrBeginFrame => {}", res);

    LayerReference render_layer_reference
    {
        .PredictedDisplayTime = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_SessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_SessionState == XR_SESSION_STATE_VISIBLE
                                || m_SessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        TRY(RenderLayer(render_layer_reference));

        render_layer_reference.Layers.push_back(
            reinterpret_cast<XrCompositionLayerBaseHeader *>(&render_layer_reference.Projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .next = nullptr,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_EnvironmentBlendMode,
        .layerCount = static_cast<uint32_t>(render_layer_reference.Layers.size()),
        .layers = render_layer_reference.Layers.data(),
    };

    if (auto res = xrEndFrame(m_Session, &frame_end_info))
        return error("xrEndFrame => {}", res);

    return ok();
}

core::result<> core::Instance::RenderLayer(LayerReference &reference)
{
    std::vector<XrView> views(m_ViewConfigurationViews.size(), { .type = XR_TYPE_VIEW });

    XrViewState view_state
    {
        .type = XR_TYPE_VIEW_STATE
    };

    XrViewLocateInfo view_locate_info
    {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .next = nullptr,
        .viewConfigurationType = m_ViewConfigurationType,
        .displayTime = reference.PredictedDisplayTime,
        .space = m_ReferenceSpace,
    };

    uint32_t view_count;
    if (auto res = xrLocateViews(
        m_Session,
        &view_locate_info,
        &view_state,
        views.size(),
        &view_count,
        views.data()))
        return error("xrLocateViews => {}", res);

    reference.Views.resize(view_count, { .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

    for (uint32_t i = 0; i < view_count; ++i)
    {
        const auto &configuration = m_ViewConfigurationViews[i];

        auto &[color, depth, framebuffers] = m_SwapchainFrames[i];

        uint32_t color_index, depth_index;

        const XrSwapchainImageAcquireInfo swapchain_image_acquire_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
            .next = nullptr,
        };

        if (auto res = xrAcquireSwapchainImage(color.Swapchain, &swapchain_image_acquire_info, &color_index))
            return error("xrAcquireSwapchainImage => {}", res);
        if (auto res = xrAcquireSwapchainImage(depth.Swapchain, &swapchain_image_acquire_info, &depth_index))
            return error("xrAcquireSwapchainImage => {}", res);

        const XrSwapchainImageWaitInfo swapchain_image_wait_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .next = nullptr,
            .timeout = XR_INFINITE_DURATION,
        };

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);
        if (auto res = xrWaitSwapchainImage(depth.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);

        const auto width = configuration.recommendedImageRectWidth;
        const auto height = configuration.recommendedImageRectHeight;

        const auto &pose = views[i].pose;
        const auto &fov = views[i].fov;

        glm::mat4 model_mat;
        {
            static auto begin = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();

            auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - begin).count();

            model_mat = { 1.0f };
            model_mat = glm::translate(model_mat, glm::vec3(0.0f, 0.0f, 0.0f));
            model_mat = glm::scale(model_mat, glm::vec3(0.1f));
            model_mat = glm::rotate(model_mat, delta * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // model_mat = glm::rotate(model_mat, delta * glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 1.0f));
            // model_mat = glm::translate(model_mat, glm::vec3(-0.5f, -0.5f, -0.5f));
        }

        glm::mat4 view_mat;
        {
            const glm::quat orientation
            {
                pose.orientation.w,
                pose.orientation.x,
                pose.orientation.y,
                pose.orientation.z,
            };

            const glm::vec3 position
            {
                pose.position.x,
                pose.position.y,
                pose.position.z,
            };

            const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
            const auto translation = glm::translate(glm::mat4(1.0f), -position);

            view_mat = rotation * translation;
        }

        glm::mat4 proj_mat;
        {
            constexpr auto near = 0.01f;
            constexpr auto far = 100.0f;

            auto l = near * tanf(fov.angleLeft);
            auto r = near * tanf(fov.angleRight);
            auto b = near * tanf(fov.angleDown);
            auto t = near * tanf(fov.angleUp);

            proj_mat = glm::frustumRH_ZO(l, r, t, b, near, far);
        }

        reference.Views[i] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .next = nullptr,
            .pose = pose,
            .fov = fov,
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

        const std::array<VkFence, 1> fences
        {
            m_InFlightFence,
        };

        if (auto res = vkWaitForFences(m_Device, fences.size(), fences.data(), true, UINT64_MAX))
            return error("vkWaitForFences => {}", res);

        vkResetFences(m_Device, fences.size(), fences.data());

        m_CameraData = {
            .model = model_mat,
            .inv_model = glm::inverse(model_mat),
            .view = view_mat,
            .inv_view = glm::inverse(view_mat),
            .proj = proj_mat,
            .inv_proj = glm::inverse(proj_mat),
        };

        {
            void *data;
            if (auto res = vkMapMemory(m_Device, m_CameraMemory, 0, sizeof(CameraData), 0, &data))
                return error("vkMapMemory => {}", res);

            memcpy(data, &m_CameraData, sizeof(CameraData));

            vkUnmapMemory(m_Device, m_CameraMemory);
        }

        const VkDescriptorBufferInfo buffer_info
        {
            .buffer = m_CameraBuffer,
            .offset = 0,
            .range = sizeof(CameraData),
        };

        const std::array writes
        {
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = m_DescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = nullptr,
            },
        };

        vkUpdateDescriptorSets(m_Device, writes.size(), writes.data(), 0, nullptr);

        TRY(RecordCommandBuffer(i, color_index));

        const std::array<VkCommandBuffer, 1> command_buffers
        {
            m_CommandBuffer,
        };

        const std::array<VkSemaphore, 1> wait_semaphores
        {
            m_ImageAvailableSemaphore,
        };

        const std::array<VkPipelineStageFlags, 1> wait_stages
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };

        const std::array<VkSemaphore, 1> signal_semaphores
        {
            m_RenderFinishedSemaphore,
        };

        const VkSubmitInfo submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = wait_semaphores.data(),
            .pWaitDstStageMask = wait_stages.data(),
            .commandBufferCount = command_buffers.size(),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = signal_semaphores.data(),
        };

        VkQueue queue;
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, m_QueueIndex, &queue);

        if (auto res = vkQueueSubmit(queue, 1, &submit_info, m_InFlightFence))
            return error("vkQueueSubmit => {}", res);

        const XrSwapchainImageReleaseInfo swapchain_image_release_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
            .next = nullptr,
        };

        if (auto res = xrReleaseSwapchainImage(color.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
        if (auto res = xrReleaseSwapchainImage(depth.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
    }

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .next = nullptr,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(reference.Views.size()),
        .views = reference.Views.data(),
    };

    return ok();
}
