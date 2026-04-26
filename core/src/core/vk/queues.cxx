#include <titan/core.hxx>

toolkit::result<> titan::Application::GetDeviceQueues()
{
    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Default,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_DefaultQueue);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_TransferQueue);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Present,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_PresentQueue);
    }

    return ok();
}
