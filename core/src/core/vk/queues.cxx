#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::GetDeviceQueues()
{
    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Default,
            .queueIndex = 0,
        };

        m_DefaultQueue = vk::GetDeviceQueue2(m_Device, device_queue_info);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
            .queueIndex = 0,
        };

        m_TransferQueue = vk::GetDeviceQueue2(m_Device, device_queue_info);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Present,
            .queueIndex = 0,
        };

        m_PresentQueue = vk::GetDeviceQueue2(m_Device, device_queue_info);
    }

    return {};
}
