#include <titan/core.hxx>

core::result<> core::Application::CreateFramebuffers()
{
    for (auto &[
             view,
             color,
             depth,
             buffer,
             framebuffers
         ] : m_SwapchainViews)
    {
        framebuffers.resize(color.Views.size());

        for (uint32_t i = 0; i < framebuffers.size(); ++i)
        {
            auto &framebuffer = framebuffers[i];

            const std::array attachments
            {
                *color.Views[i],
                *depth.Views[i],
            };

            const VkFramebufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_RenderPass,
                .attachmentCount = attachments.size(),
                .pAttachments = attachments.data(),
                .width = view.recommendedImageRectWidth,
                .height = view.recommendedImageRectHeight,
                .layers = 1,
            };

            if (auto res = vk::Framebuffer::create(m_Device, create_info) >> framebuffer)
                return res;
        }
    }

    {
        auto &[
            width,
            height,
            color,
            depth
        ] = m_WindowSwapchainView;

        m_Frames.resize(color.Views.size());

        for (uint32_t i = 0; i < m_Frames.size(); ++i)
        {
            auto &framebuffer = m_Frames[i].Framebuffer;

            const std::array attachments
            {
                *color.Views[i],
                *depth.Views[i],
            };

            const VkFramebufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_RenderPass,
                .attachmentCount = attachments.size(),
                .pAttachments = attachments.data(),
                .width = width,
                .height = height,
                .layers = 1,
            };

            if (auto res = vk::Framebuffer::create(m_Device, create_info) >> framebuffer)
                return res;
        }
    }

    return ok();
}
