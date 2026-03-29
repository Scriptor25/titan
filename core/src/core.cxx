#include <titan/core.hxx>
#include <titan/log.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstring>
#include <utility>

core::Application::Application(ApplicationInfo info)
    : m_Info(std::move(info))
{
}

core::result<> core::Application::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    m_Mesh = obj::Open("res/mesh/teapot.obj");

    TRY(InitializeWindow());
    TRY(InitializeAudio());
    TRY(InitializeGraphics());

    OnStart();

    return ok();
}

void core::Application::Terminate()
{
    m_Window.Close();
}

core::result<bool> core::Application::Spin()
{
    glfw::PollEvents();

    if (m_Window.ShouldClose())
    {
        OnStop();
        return false;
    }

    TRY_CAST(PollEvents(), bool);
    TRY_CAST(RenderFrame(), bool);

    return true;
}

core::result<> core::Application::InitializeGraphics()
{
    TRY(CreateXrInstance());
    TRY(CreateXrMessenger());
    TRY(GetSystemId());
    TRY(CreateVkInstance());
    TRY(CreateVkMessenger());
    TRY(GetPhysicalDevice());
    TRY(CreateWindowSurface());
    TRY(GetQueueFamilyIndices());
    TRY(CreateDevice());
    TRY(CreateWindowSwapchain());
    TRY(GetDeviceQueues());
    TRY(CreateSession());
    TRY(GetViewConfigurationType());
    TRY(GetViewConfigurationViews());
    TRY(GetFormats());
    TRY(CreateSwapchains());
    TRY(GetEnvironmentBlendMode());
    TRY(CreateReferenceSpace());
    TRY(CreateRenderPass());
    TRY(CreateFramebuffers());
    TRY(CreatePipelineCache());
    TRY(CreateDescriptorPool());
    TRY(CreateDescriptorSetLayouts());
    TRY(CreateDescriptorSet());
    TRY(CreatePipelineLayout());
    TRY(CreatePipeline());
    TRY(CreateCommandPools());
    TRY(CreateCommandBuffers());
    TRY(CreateSynchronization());
    TRY(CreateVertexBuffer());
    TRY(CreateVertexMemory());
    TRY(FillVertexBuffer());

    return ok();
}

core::result<> core::Application::PollEvents()
{
    XrEventDataBuffer event_data
    {
        .type = XR_TYPE_EVENT_DATA_BUFFER,
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

core::result<> core::Application::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    XrFrameState frame_state
    {
        .type = XR_TYPE_FRAME_STATE,
        .predictedDisplayTime = {},
        .predictedDisplayPeriod = {},
        .shouldRender = {},
    };

    if (auto res = xrWaitFrame(m_Session, &frame_wait_info, &frame_state))
        return error("xrWaitFrame => {}", res);

    PreFrame();

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    if (auto res = xrBeginFrame(m_Session, &frame_begin_info))
        return error("xrBeginFrame => {}", res);

    OnFrame();

    RenderLayerInfo render_layer_info
    {
        .PredictedDisplayTime = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_SessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_SessionState == XR_SESSION_STATE_VISIBLE
                                || m_SessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        TRY(RenderLayer(render_layer_info));

        render_layer_info.Layers.push_back(
            reinterpret_cast<XrCompositionLayerBaseHeader *>(&render_layer_info.Projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_EnvironmentBlendMode,
        .layerCount = static_cast<uint32_t>(render_layer_info.Layers.size()),
        .layers = render_layer_info.Layers.data(),
    };

    if (auto res = xrEndFrame(m_Session, &frame_end_info))
        return error("xrEndFrame => {}", res);

    PostFrame();

    return ok();
}

core::result<> core::Application::RenderLayer(RenderLayerInfo &reference)
{
    std::vector<XrView> views(m_ViewConfigurationViews.size(), { .type = XR_TYPE_VIEW });
    XrViewState view_state{ .type = XR_TYPE_VIEW_STATE };

    const XrViewLocateInfo view_locate_info
    {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
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

    std::optional<BlitViewInfo> blit_view_info;

    if (auto res = vkWaitForFences(
        m_Device,
        1,
        &*m_Fence,
        true,
        std::numeric_limits<uint64_t>::max()))
        return error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        1,
        &*m_Fence))
        return error("vkResetFences => {}", res);

    for (uint32_t view_index = 0; view_index < view_count; ++view_index)
    {
        const auto &view = views[view_index];
        const auto &view_configuration_view = m_ViewConfigurationViews[view_index];

        auto &[color, depth, _0, _1] = m_SwapchainViews[view_index];

        const XrSwapchainImageAcquireInfo swapchain_image_acquire_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
        };

        uint32_t color_index, depth_index;
        if (auto res = xrAcquireSwapchainImage(color.Swapchain, &swapchain_image_acquire_info, &color_index))
            return error("xrAcquireSwapchainImage => {}", res);
        if (auto res = xrAcquireSwapchainImage(depth.Swapchain, &swapchain_image_acquire_info, &depth_index))
            return error("xrAcquireSwapchainImage => {}", res);

        const XrSwapchainImageWaitInfo swapchain_image_wait_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .timeout = XR_INFINITE_DURATION,
        };

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);
        if (auto res = xrWaitSwapchainImage(depth.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);

        const auto &pose = view.pose;
        const auto &fov = view.fov;

        const auto &width = view_configuration_view.recommendedImageRectWidth;
        const auto &height = view_configuration_view.recommendedImageRectHeight;

        reference.Views[view_index] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
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

        if (!blit_view_info)
            blit_view_info = {
                .Index = color_index,
                .Width = width,
                .Height = height,
            };

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

        const CameraData camera_data
        {
            .Screen = proj_mat * view_mat * model_mat,
            .Normal = glm::transpose(glm::inverse(model_mat)),
        };

        TRY(RecordCommandBuffer(view_index, color_index, camera_data));
    }

    {
        std::vector<VkCommandBufferSubmitInfo> command_buffers(view_count);
        for (uint32_t i = 0; i < view_count; ++i)
            command_buffers[i] = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = m_SwapchainViews[i].Buffer,
            };

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
    }

    for (std::uint32_t view_index = 0; view_index < view_count; ++view_index)
    {
        auto &[color, depth, _0, _1] = m_SwapchainViews[view_index];

        const XrSwapchainImageReleaseInfo swapchain_image_release_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
        };

        if (auto res = xrReleaseSwapchainImage(color.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
        if (auto res = xrReleaseSwapchainImage(depth.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
    }

    TRY(BlitView(*blit_view_info));

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(reference.Views.size()),
        .views = reference.Views.data(),
    };

    return ok();
}

core::result<> core::Application::BlitView(const BlitViewInfo &info)
{
    auto &[available, finished, fence, buffer] = m_Frames[m_FrameIndex];
    m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();

    if (auto res = vkWaitForFences(
        m_Device,
        1,
        &*fence,
        true,
        std::numeric_limits<uint64_t>::max()))
        return error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        1,
        &*fence))
        return error("vkResetFences => {}", res);

    uint32_t image_index;
    if (auto res = vkAcquireNextImageKHR(
        m_Device,
        m_WindowSwapchain.Swapchain,
        std::numeric_limits<uint64_t>::max(),
        available,
        nullptr,
        &image_index))
        return error("vkAcquireNextImageKHR => {}", res);

    if (auto res = vkResetCommandBuffer(buffer, 0))
        return error("vkResetCommandBuffer => {}", res);

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (auto res = vkBeginCommandBuffer(buffer, &command_buffer_begin_info))
        return error("vkBeginCommandBuffer => {}", res);

    auto &src_image = m_SwapchainViews[0].Color.Images[info.Index];
    auto &dst_image = m_WindowSwapchain.Images[image_index];

    {
        const VkImageMemoryBarrier2 src_image_memory_barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = src_image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const VkImageMemoryBarrier2 dst_image_memory_barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = dst_image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const std::array image_memory_barriers
        {
            src_image_memory_barrier,
            dst_image_memory_barrier,
        };

        const VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = image_memory_barriers.size(),
            .pImageMemoryBarriers = image_memory_barriers.data(),
        };

        vkCmdPipelineBarrier2(buffer, &dependency_info);
    }

    {
        const std::array regions
        {
            VkImageBlit2
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .srcOffsets = {
                    { 0, 0, 0 },
                    {
                        static_cast<int32_t>(info.Width),
                        static_cast<int32_t>(info.Height),
                        1,
                    },
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .dstOffsets = {
                    { 0, 0, 0 },
                    {
                        static_cast<int32_t>(m_WindowSwapchain.Width),
                        static_cast<int32_t>(m_WindowSwapchain.Height),
                        1,
                    },
                },
            }
        };

        const VkBlitImageInfo2 blit_image_info
        {
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .srcImage = src_image,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = dst_image,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = regions.size(),
            .pRegions = regions.data(),
            .filter = VK_FILTER_LINEAR,
        };

        vkCmdBlitImage2(buffer, &blit_image_info);
    }

    {
        const VkImageMemoryBarrier2 src_image_memory_barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = src_image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const VkImageMemoryBarrier2 dst_image_memory_barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = dst_image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const std::array image_memory_barriers
        {
            src_image_memory_barrier,
            dst_image_memory_barrier,
        };

        const VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = image_memory_barriers.size(),
            .pImageMemoryBarriers = image_memory_barriers.data(),
        };

        vkCmdPipelineBarrier2(buffer, &dependency_info);
    }

    if (auto res = vkEndCommandBuffer(buffer))
        return error("vkEndCommandBuffer => {}", res);

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

        if (auto res = vkQueueSubmit2(m_TransferQueue, submits.size(), submits.data(), fence))
            return error("vkQueueSubmit2 => {}", res);
    }

    {
        const std::array wait_semaphores
        {
            *finished,
        };

        const std::array swapchains
        {
            *m_WindowSwapchain.Swapchain,
        };

        const VkPresentInfoKHR present_info
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = wait_semaphores.size(),
            .pWaitSemaphores = wait_semaphores.data(),
            .swapchainCount = swapchains.size(),
            .pSwapchains = swapchains.data(),
            .pImageIndices = &image_index,
        };

        if (auto res = vkQueuePresentKHR(m_PresentQueue, &present_info))
            return error("vkQueuePresentKHR =>{}", res);
    }

    return ok();
}

void core::Application::OnStart()
{
}

void core::Application::PreFrame()
{
}

void core::Application::OnFrame()
{
}

void core::Application::PostFrame()
{
}

void core::Application::OnStop()
{
}
