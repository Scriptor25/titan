#include <titan/core.hxx>

core::result<> core::Application::AllocateCommandBuffers()
{
    return ok()
           & [&]
           {
               const VkCommandBufferAllocateInfo allocate_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                   .commandPool = m_DefaultPool,
                   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                   .commandBufferCount = static_cast<uint32_t>(m_SwapchainViews.size()),
               };

               return vk::CommandBuffer::create_collection(m_Device, allocate_info);
           }
           & [&](std::vector<vk::CommandBuffer> &&buffers)
           {
               for (uint32_t i = 0; i < buffers.size(); ++i)
                   m_SwapchainViews[i].Buffer = std::move(buffers[i]);
               return ok();
           }
           & [&]
           {
               const VkCommandBufferAllocateInfo allocate_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                   .commandPool = m_TransferPool,
                   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                   .commandBufferCount = static_cast<uint32_t>(m_Frames.size()),
               };

               return vk::CommandBuffer::create_collection(m_Device, allocate_info);
           }
           & [&](std::vector<vk::CommandBuffer> &&buffers)
           {
               for (uint32_t i = 0; i < buffers.size(); ++i)
                   m_Frames[i].Buffer = std::move(buffers[i]);
               return ok();
           };
}
