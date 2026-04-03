#include <titan/core.hxx>

core::result<> core::Application::AllocateCommandBuffers()
{
    {
        const VkCommandBufferAllocateInfo allocate_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_DefaultPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(m_SwapchainViews.size()),
        };

        auto set_buffers = [this](std::vector<vk::CommandBuffer> &&buffers)
        {
            for (uint32_t i = 0; i < buffers.size(); ++i)
                m_SwapchainViews[i].Buffer = std::move(buffers[i]);
            return ok();
        };

        TRY(vk::CommandBuffer::allocate2(m_Device, allocate_info) | set_buffers);
    }

    {
        const VkCommandBufferAllocateInfo allocate_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_TransferPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(m_Frames.size()),
        };

        auto set_buffers = [this](std::vector<vk::CommandBuffer> &&buffers)
        {
            for (uint32_t i = 0; i < buffers.size(); ++i)
                m_Frames[i].Buffer = std::move(buffers[i]);
            return ok();
        };

        TRY(vk::CommandBuffer::allocate2(m_Device, allocate_info) | set_buffers);
    }

    return ok();
}
