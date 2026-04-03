#include <titan/core.hxx>

core::result<> core::Application::CreateDescriptorSetLayouts()
{
    // {
    //     const std::array bindings
    //     {
    //         VkDescriptorSetLayoutBinding
    //         {
    //             .binding = 0,
    //             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //             .descriptorCount = 1,
    //             .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //         },
    //     };
    //
    //     const VkDescriptorSetLayoutCreateInfo create_info
    //     {
    //         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //         .bindingCount = bindings.size(),
    //         .pBindings = bindings.data(),
    //     };
    //
    //     TRY(vk::DescriptorSetLayout::create(m_Device, create_info) >> m_DescriptorSetLayouts.emplace_back());
    // }

    return ok();
}
