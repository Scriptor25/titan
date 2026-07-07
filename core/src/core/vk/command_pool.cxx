#include <titan/core.hxx>

toolkit::result<> titan::Application::CreateCommandPools()
{
    HANDLE(CreateCommandPool(m_QueueFamilyIndices.Default) >> m_DefaultPool);
    HANDLE(CreateCommandPool(m_QueueFamilyIndices.Transfer) >> m_TransferPool);

    return {};
}

toolkit::result<titan::vk::CommandPool> titan::Application::CreateCommandPool(uint32_t queue_family_index)
{
    const VkCommandPoolCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index,
    };

    return vk::CommandPool::create(m_Device, create_info);
}
