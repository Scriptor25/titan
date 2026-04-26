#include <titan/core.hxx>

toolkit::result<> titan::Application::CreateCommandPools()
{
    return ok()
           & [&]
           {
               const VkCommandPoolCreateInfo create_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                   .queueFamilyIndex = m_QueueFamilyIndices.Default,
               };

               return vk::CommandPool::create(m_Device, create_info) >> m_DefaultPool;
           }
           & [&]
           {
               const VkCommandPoolCreateInfo create_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                   .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
               };

               return vk::CommandPool::create(m_Device, create_info) >> m_TransferPool;
           };
}
