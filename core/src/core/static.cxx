#include <titan/core.hxx>

#include <fstream>

void core::Instance::GetInstanceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_INSTANCE_EXTENSIONS.begin(), VK_INSTANCE_EXTENSIONS.end());

    uint32_t glfw_extension_count;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    dst.insert(dst.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
}

void core::Instance::GetDeviceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_DEVICE_EXTENSIONS.begin(), VK_DEVICE_EXTENSIONS.end());
}

core::result<> core::Instance::FindFormats(
    VkPhysicalDevice physical_device,
    const std::vector<int64_t> &formats,
    const std::vector<FormatReference> &references)
{
    std::map<VkFormat, VkFormatProperties> properties;

    for (const auto format : formats)
        vkGetPhysicalDeviceFormatProperties(
            physical_device,
            static_cast<VkFormat>(format),
            &properties[static_cast<VkFormat>(format)]);

    auto count = 0;

    for (const auto format : formats)
        for (auto &reference : references)
            if (!reference.format
                && properties[static_cast<VkFormat>(format)].optimalTilingFeatures & reference.features)
            {
                reference.format = static_cast<VkFormat>(format);
                count++;
            }

    if (count >= references.size())
        return ok();

    for (const auto format : formats)
        for (auto &reference : references)
            if (!reference.format
                && properties[static_cast<VkFormat>(format)].linearTilingFeatures & reference.features)
            {
                reference.format = static_cast<VkFormat>(format);
                count++;
            }

    if (count >= references.size())
        return ok();

    std::vector<std::string_view> missing;
    for (auto &reference : references)
        if (!reference.format)
            missing.push_back(reference.name);

    return error("failed to find formats for references {}.", missing);
}

core::result<uint32_t> core::Instance::FindMemoryType(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags type_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        auto &[flags, _1] = memory_properties.memoryTypes[i];
        if (type_filter & 1 << i && type_flags & flags)
            return i;
    }

    return error<uint32_t>("failed to find any suitable memory type.");
}

std::vector<char> core::Instance::LoadShaderModuleBinary(const std::filesystem::path &path)
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
