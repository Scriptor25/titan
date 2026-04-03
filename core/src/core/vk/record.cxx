#include <titan/core.hxx>

core::result<> core::Application::RecordCommandBuffer(
    const uint32_t width,
    const uint32_t height,
    const CameraData &camera_data,
    vk::CommandBuffer &buffer,
    vk::Framebuffer &framebuffer)
{
    if (auto res = vkResetCommandBuffer(buffer, 0))
        return error("vkResetCommandBuffer => {}", res);

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (auto res = vkBeginCommandBuffer(buffer, &command_buffer_begin_info))
        return error("vkBeginCommandBuffer => {}", res);

    const VkViewport viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor
    {
        .offset = {
            .x = 0,
            .y = 0,
        },
        .extent = {
            .width = width,
            .height = height,
        },
    };

    const std::array clear_values
    {
        VkClearValue{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
        VkClearValue{ .depthStencil = { .depth = 1.0f } },
    };

    const VkRenderPassBeginInfo render_pass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_RenderPass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = width,
                .height = height,
            },
        },
        .clearValueCount = clear_values.size(),
        .pClearValues = clear_values.data(),
    };

    const VkSubpassBeginInfo subpass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
        .contents = VK_SUBPASS_CONTENTS_INLINE,
    };

    vkCmdBeginRenderPass2(buffer, &render_pass_begin_info, &subpass_begin_info);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdSetViewportWithCount(buffer, 1, &viewport);
    vkCmdSetScissorWithCount(buffer, 1, &scissor);

    const std::array buffers
    {
        *m_VertexBuffer,
    };

    const std::array offsets
    {
        0LU,
    };

    vkCmdBindVertexBuffers(buffer, 0, buffers.size(), buffers.data(), offsets.data());

    if (!m_DescriptorSets.empty())
    {
        std::vector<VkDescriptorSet> descriptor_sets(m_DescriptorSets.size());
        for (uint32_t i = 0; i < m_DescriptorSets.size(); ++i)
            descriptor_sets[i] = m_DescriptorSets[i];

        const VkBindDescriptorSetsInfo bind_descriptor_sets_info
        {
            .sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .layout = m_PipelineLayout,
            .firstSet = 0,
            .descriptorSetCount = static_cast<uint32_t>(descriptor_sets.size()),
            .pDescriptorSets = descriptor_sets.data(),
        };

        vkCmdBindDescriptorSets2(buffer, &bind_descriptor_sets_info);
    }

    const VkPushConstantsInfo push_constants_info
    {
        .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
        .layout = m_PipelineLayout,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(CameraData),
        .pValues = &camera_data,
    };

    vkCmdPushConstants2(buffer, &push_constants_info);

    vkCmdDraw(buffer, m_Mesh.Vertices.size(), 1, 0, 0);

    const VkSubpassEndInfo subpass_end_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
    };

    vkCmdEndRenderPass2(buffer, &subpass_end_info);

    if (auto res = vkEndCommandBuffer(buffer))
        return error("vkEndCommandBuffer => {}", res);

    return ok();
}
