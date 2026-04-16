#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <set>

titan::result<> titan::Application::GetQueueFamilyIndices()
{
    std::optional<uint32_t> index_default, index_graphics, index_compute, index_transfer, index_present;

    auto queue_family_properties = vk::GetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice);

    for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
    {
        auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

        if (!count)
            continue;

        if (!index_default
            && flags & VK_QUEUE_GRAPHICS_BIT
            && flags & VK_QUEUE_COMPUTE_BIT
            && flags & VK_QUEUE_TRANSFER_BIT)
            index_default = i;

        if (!index_graphics && flags & VK_QUEUE_GRAPHICS_BIT)
            index_graphics = i;
        if (!index_compute && flags & VK_QUEUE_COMPUTE_BIT)
            index_compute = i;
        if (!index_transfer && flags & VK_QUEUE_TRANSFER_BIT)
            index_transfer = i;

        if (!index_present)
        {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_WindowSurface, &supported);

            if (supported)
                index_present = i;
        }
    }

    if (!index_default)
    {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

            if (!count)
                continue;

            if (!index_default
                && flags & VK_QUEUE_GRAPHICS_BIT
                && flags & VK_QUEUE_COMPUTE_BIT)
            {
                index_default = i;
                break;
            }
        }
    }

    if (!index_default)
    {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

            if (!count)
                continue;

            if (!index_default && flags & VK_QUEUE_GRAPHICS_BIT)
            {
                index_default = i;
                break;
            }
        }
    }

    std::set<std::string> missing;
    if (!index_default)
        missing.insert("default");
    if (!index_graphics)
        missing.insert("graphics");
    if (!index_compute)
        missing.insert("compute");
    if (!index_transfer)
        missing.insert("transfer");
    if (!index_present)
        missing.insert("present");

    if (missing.empty())
    {
        m_QueueFamilyIndices = {
            .Default = *index_default,
            .Graphics = *index_graphics,
            .Compute = *index_compute,
            .Transfer = *index_transfer,
            .Present = *index_present,
        };
        return ok();
    }

    return error("failed to find any suitable queue family for {}.", missing);
}
