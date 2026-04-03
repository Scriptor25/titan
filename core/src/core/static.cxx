#include <titan/core.hxx>

#include <fstream>

void core::Application::GetInstanceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_INSTANCE_EXTENSIONS.begin(), VK_INSTANCE_EXTENSIONS.end());

    uint32_t glfw_extension_count;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    dst.insert(dst.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
}

void core::Application::GetDeviceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_DEVICE_EXTENSIONS.begin(), VK_DEVICE_EXTENSIONS.end());
}

core::result<> core::Application::FindFormats(
    VkPhysicalDevice physical_device,
    const std::vector<VkFormat> &formats,
    const std::vector<FormatReference> &references)
{
    auto count = 0;

    for (const auto format : formats)
    {
        VkFormatProperties2 properties
        {
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
        };

        vkGetPhysicalDeviceFormatProperties2(physical_device, format, &properties);

        auto &format_properties = properties.formatProperties;

        for (auto &reference : references)
        {
            if (reference.Format)
                continue;

            switch (reference.Tiling)
            {
            case VK_IMAGE_TILING_LINEAR:
                if (format_properties.linearTilingFeatures & reference.Features)
                {
                    reference.Format = format;
                    count++;
                }
                break;

            case VK_IMAGE_TILING_OPTIMAL:
                if (format_properties.optimalTilingFeatures & reference.Features)
                {
                    reference.Format = format;
                    count++;
                }
                break;

            default:
                break;
            }
        }
    }

    if (count >= references.size())
        return ok();

    std::vector<std::string_view> missing;
    for (auto &reference : references)
        if (!reference.Format)
            missing.push_back(reference.Name);

    return error("failed to find formats for references {}.", missing);
}

core::result<uint32_t> core::Application::FindMemoryType(
    VkPhysicalDevice physical_device,
    const uint32_t type_filter,
    const VkMemoryPropertyFlags type_flags)
{
    VkPhysicalDeviceMemoryProperties2 memory_properties
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
    };
    vkGetPhysicalDeviceMemoryProperties2(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryProperties.memoryTypeCount; ++i)
    {
        auto &[property_flags, heap_index] = memory_properties.memoryProperties.memoryTypes[i];
        if (type_filter & 1 << i && type_flags & property_flags)
            return i;
    }

    return error<uint32_t>("failed to find any suitable memory type.");
}

std::vector<char> core::Application::LoadShaderModuleBinary(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return {};

    const auto count = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<char> binary(count);
    stream.read(binary.data(), static_cast<std::streamsize>(binary.size()));

    return binary;
}
