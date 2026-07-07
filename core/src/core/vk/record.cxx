#include <titan/component.hxx>
#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::RecordCommandBuffer(
    const uint32_t width,
    const uint32_t height,
    const glm::mat4 &screen_matrix,
    vk::CommandBuffer &buffer,
    vk::Framebuffer &framebuffer)
{
    HANDLE(vk::ResetCommandBuffer(buffer, 0));

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    HANDLE(vk::BeginCommandBuffer(buffer, command_buffer_begin_info));

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

    for (auto [entity, transform, mesh] : m_Entities.Query<component::Transform, component::Mesh>())
    {
        if (!entity.Active)
            continue;

        auto &info = m_Meshes[mesh.Resource];

        const std::array<VkBuffer, 1> buffers
        {
            info.VertexBuffer,
        };

        const std::array offsets
        {
            info.VertexBufferOffset,
        };

        vkCmdBindVertexBuffers(buffer, 0, buffers.size(), buffers.data(), offsets.data());
        vkCmdBindIndexBuffer(buffer, info.IndexBuffer, info.IndexBufferOffset, info.IndexType);

        const ShaderData shader_data
        {
            .Screen = screen_matrix,
            .Model = transform.Matrix,
            .Normal = glm::transpose(transform.Inverse),
        };

        const VkPushConstantsInfo push_constants_info
        {
            .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
            .layout = m_PipelineLayout,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ShaderData),
            .pValues = &shader_data,
        };

        vkCmdPushConstants2(buffer, &push_constants_info);

        vkCmdDrawIndexed(buffer, info.IndexCount, 1, 0, 0, 0);
    }

    const VkSubpassEndInfo subpass_end_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
    };

    vkCmdEndRenderPass2(buffer, &subpass_end_info);

    return vk::EndCommandBuffer(buffer);
}
