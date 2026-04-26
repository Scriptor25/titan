#include <titan/core.hxx>

toolkit::result<> titan::Application::GetFormats()
{
    std::vector<VkFormat> formats;
    formats.insert(formats.end(), VK_COLOR_FORMATS.begin(), VK_COLOR_FORMATS.end());
    formats.insert(formats.end(), VK_DEPTH_FORMATS.begin(), VK_DEPTH_FORMATS.end());

    return FindFormats(
        m_PhysicalDevice,
        formats,
        {
            {
                .Name = "color",
                .Format = m_ColorFormat,
                .Tiling = VK_IMAGE_TILING_OPTIMAL,
                .Features = VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT,
            },
            {
                .Name = "depth",
                .Format = m_DepthFormat,
                .Tiling = VK_IMAGE_TILING_OPTIMAL,
                .Features = VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT,
            }
        });
}
