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

    TRY(vk::Fence::create(m_Device, fence_create_info) >> m_Fence);

    for (auto &[
             available,
             finished,
             fence,
             buffer,
             framebuffer
         ] : m_Frames)
    {
        TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> available);
        TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> finished);
        TRY(vk::Fence::create(m_Device, fence_create_info) >> fence);
    }

    return ok();
}
