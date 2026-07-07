#include <titan/core.hxx>

toolkit::result<> titan::Application::AllocateCommandBuffers()
{
    {
        std::vector<vk::CommandBuffer> buffers;
        HANDLE(AllocateCommandBuffers(m_DefaultPool, m_SwapchainViews.size()) >> buffers);

        for (uint32_t i = 0; i < buffers.size(); ++i)
            m_SwapchainViews[i].Buffer = std::move(buffers[i]);
    }

    {
        std::vector<vk::CommandBuffer> buffers;
        HANDLE(AllocateCommandBuffers(m_TransferPool, m_Frames.size()) >> buffers);

        for (uint32_t i = 0; i < buffers.size(); ++i)
            m_Frames[i].Buffer = std::move(buffers[i]);
    }

    return {};
}

toolkit::result<std::vector<titan::vk::CommandBuffer>> titan::Application::AllocateCommandBuffers(
    VkCommandPool pool,
    const uint32_t count)
{
    const VkCommandBufferAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    return vk::CommandBuffer::create_collection(m_Device, allocate_info);
}
