#include <titan/core.hxx>

core::result<> core::Application::CreateSynchronization()
{
    const VkSemaphoreCreateInfo semaphore_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    const VkFenceCreateInfo fence_create_info
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (auto res = vk::Fence::create(m_Device, fence_create_info) >> m_Fence)
        return res;

    for (auto &[
             available,
             finished,
             fence,
             buffer,
             framebuffer
         ] : m_Frames)
    {
        if (auto res = vk::Semaphore::create(m_Device, semaphore_create_info) >> available)
            return res;
        if (auto res = vk::Semaphore::create(m_Device, semaphore_create_info) >> finished)
            return res;
        if (auto res = vk::Fence::create(m_Device, fence_create_info) >> fence)
            return res;
    }

    return ok();
}
