#include <titan/component.hxx>
#include <titan/core.hxx>
#include <titan/format.hxx>
#include <titan/utils.hxx>
#include <titan/system/graphics.hxx>

#include <pkg/mesh.hxx>
#include <pkg/shader.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

void titan::GraphicsSystem::GetInstanceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_INSTANCE_EXTENSIONS.begin(), VK_INSTANCE_EXTENSIONS.end());

    uint32_t glfw_extension_count;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    dst.insert(dst.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
}

void titan::GraphicsSystem::GetDeviceExtensions(std::vector<const char *> &dst)
{
    dst.insert(dst.end(), VK_DEVICE_EXTENSIONS.begin(), VK_DEVICE_EXTENSIONS.end());
}

toolkit::result<> titan::GraphicsSystem::FindFormats(
    VkPhysicalDevice physical_device,
    const std::vector<VkFormat> &formats,
    const std::vector<detail::FormatReference> &references)
{
    auto count = 0;

    for (const auto format : formats)
    {
        VkFormatProperties2 properties
        {
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
        };

        vkGetPhysicalDeviceFormatProperties2(physical_device, format, &properties);

        const auto &format_properties = properties.formatProperties;

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
        return {};

    std::vector<std::string_view> missing;
    for (auto &reference : references)
        if (!reference.Format)
            missing.push_back(reference.Name);

    return toolkit::make_error("failed to find formats for references {}.", missing);
}

toolkit::result<uint32_t> titan::GraphicsSystem::FindMemoryType(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags type_flags)
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

    return toolkit::make_error("failed to find any suitable memory type.");
}

titan::GraphicsSystem::GraphicsSystem(Application &application)
    : m_Application(application)
{
}

titan::GraphicsSystem::~GraphicsSystem()
{
}

toolkit::result<> titan::GraphicsSystem::Initialize()
{
    return sequence(
        this,

        &GraphicsSystem::CreateInstance,
        &GraphicsSystem::CreateMessenger,

        &GraphicsSystem::CreatePhysicalDevice,
        &GraphicsSystem::CreateFormats,
        &GraphicsSystem::CreateWindowSurface,
        &GraphicsSystem::CreateQueueFamilyIndices,

        &GraphicsSystem::CreateDevice,
        &GraphicsSystem::CreateDeviceQueues,

        &GraphicsSystem::CreateWindowSwapchainView,

        &GraphicsSystem::CreateSession,
        &GraphicsSystem::CreateSpaces,

        &GraphicsSystem::CreateCommandPools,

        &GraphicsSystem::CreateRenderPass,

        &GraphicsSystem::CreatePipelineCache,
        &GraphicsSystem::CreatePipelineLayout,
        &GraphicsSystem::CreatePipeline,

        &GraphicsSystem::CreateBuffers,
        &GraphicsSystem::FillBuffers,

        &GraphicsSystem::CreateSwapchainViews,
        &GraphicsSystem::CreateFrames,

        &GraphicsSystem::CreateSynchronization
    );
}

toolkit::result<> titan::GraphicsSystem::Destroy()
{
    if (!m_Device)
        return {};

    if (auto res = vkDeviceWaitIdle(m_Device))
        return toolkit::make_error("vkDeviceWaitIdle => {}", res);

    if (auto res = StorePipelineCache(); !res)
        return res;

    return {};
}

toolkit::result<> titan::GraphicsSystem::RenderFrame()
{
    return {};
}

toolkit::result<> titan::GraphicsSystem::CreatePhysicalDevice()
{
    return toolkit::result()
           & [&]
           {
               const XrVulkanGraphicsDeviceGetInfoKHR get_info
               {
                   .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
                   .systemId = m_Application.GetXrSystemId(),
                   .vulkanInstance = m_Instance,
               };

               return xr::GetVulkanGraphicsDevice2KHR(m_Application.GetXrInstance(), get_info);
           }
           & [&](VkPhysicalDevice physical_device) -> toolkit::result<>
           {
               m_PhysicalDevice = vk::PhysicalDevice::wrap(physical_device);

               const auto properties = vk::GetPhysicalDeviceProperties2(m_PhysicalDevice).properties;

               if (sizeof(detail::ShaderData) <= properties.limits.maxPushConstantsSize)
                   return {};

               return toolkit::make_error(
                   "shader data struct size is greater than physical device max push constants size ({} > {}).",
                   sizeof(detail::ShaderData),
                   properties.limits.maxPushConstantsSize);
           };
}

toolkit::result<> titan::GraphicsSystem::CreateFormats()
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

toolkit::result<> titan::GraphicsSystem::CreateQueueFamilyIndices()
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
        return {};
    }

    return toolkit::make_error("failed to find any suitable queue family for {}.", missing);
}

toolkit::result<> titan::GraphicsSystem::CreateDevice()
{
    std::vector<const char *> extensions;
    GetDeviceExtensions(extensions);

    auto queue_priority = 1.0f;

    const std::set<uint32_t> queue_family_indices
    {
        reinterpret_cast<uint32_t *>(&m_QueueFamilyIndices),
        reinterpret_cast<uint32_t *>(&m_QueueFamilyIndices + 1)
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    for (auto &queue_family_index : queue_family_indices)
        queue_create_infos.push_back(
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            });

    auto physical_device_features = vk::GetPhysicalDeviceFeatures2(m_PhysicalDevice);

    VkPhysicalDeviceSynchronization2Features synchronization2_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };

    VkPhysicalDeviceMultiviewFeatures multiview_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
        .pNext = &synchronization2_features,
        .multiview = true,
        .multiviewGeometryShader = false,
        .multiviewTessellationShader = false,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &multiview_features,
        .bufferDeviceAddress = true,
    };

    const VkDeviceCreateInfo device_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &buffer_device_address_features,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = VK_DEVICE_LAYERS.size(),
        .ppEnabledLayerNames = VK_DEVICE_LAYERS.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &physical_device_features.features,
    };

    const XrVulkanDeviceCreateInfoKHR create_info
    {
        .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
        .systemId = m_Application.GetXrSystemId(),
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanPhysicalDevice = m_PhysicalDevice,
        .vulkanCreateInfo = &device_create_info,
    };

    return xr::CreateVulkanDeviceKHR(m_Application.GetXrInstance(), create_info) >> m_Device;
}

toolkit::result<> titan::GraphicsSystem::CreateDeviceQueues()
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

toolkit::result<> titan::GraphicsSystem::CreateCommandPools()
{
    return toolkit::result()
           & [&]
           {
               const VkCommandPoolCreateInfo create_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                   .queueFamilyIndex = m_QueueFamilyIndices.Default,
               };

               return vk::CommandPool::create(m_Device, create_info) >> m_DefaultPool;
           }
           & [&]
           {
               const VkCommandPoolCreateInfo create_info
               {
                   .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                   .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
               };

               return vk::CommandPool::create(m_Device, create_info) >> m_TransferPool;
           };
}

toolkit::result<> titan::GraphicsSystem::CreatePipelineCache()
{
    std::vector<char> data;
    if (auto res = m_Application.LoadBinary("pipeline-cache") >> data; !res)
        return res;

    const VkPipelineCacheCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .initialDataSize = data.size(),
        .pInitialData = data.data(),
    };

    return vk::PipelineCache::create(m_Device, create_info) >> m_PipelineCache;
}

toolkit::result<> titan::GraphicsSystem::StorePipelineCache()
{
    return vk::GetPipelineCacheData(m_Device, m_PipelineCache)
           & [&](std::vector<char> &&data) -> toolkit::result<>
           {
               return m_Application.StoreBinary("pipeline-cache", data);
           };
}

toolkit::result<> titan::GraphicsSystem::CreateSession()
{
    const XrGraphicsBindingVulkan2KHR graphics_binding_vulkan
    {
        .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
        .instance = m_Instance,
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .queueFamilyIndex = m_QueueFamilyIndices.Default,
        .queueIndex = 0,
    };

    const XrSessionCreateInfo create_info
    {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &graphics_binding_vulkan,
        .systemId = m_Application.GetXrSystemId(),
    };

    return xr::Session::create(m_Application.GetXrInstance(), create_info) >> m_Session;
}

toolkit::result<> titan::GraphicsSystem::CreateSpaces()
{
    return toolkit::result()
           & [&]
           {
               const XrReferenceSpaceCreateInfo create_info
               {
                   .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                   .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
                   .poseInReferenceSpace = {
                       .orientation = {
                           .x = 0.0f,
                           .y = 0.0f,
                           .z = 0.0f,
                           .w = 1.0f,
                       },
                       .position = {
                           .x = 0.0f,
                           .y = 0.0f,
                           .z = 0.0f,
                       },
                   },
               };

               return xr::ReferenceSpace::create(m_Session, create_info) >> m_ViewSpace;
           }
           & [&]
           {
               const XrReferenceSpaceCreateInfo create_info
               {
                   .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                   .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
                   .poseInReferenceSpace = {
                       .orientation = {
                           .x = 0.0f,
                           .y = 0.0f,
                           .z = 0.0f,
                           .w = 1.0f,
                       },
                       .position = {
                           .x = 0.0f,
                           .y = 0.0f,
                           .z = 0.0f,
                       },
                   },
               };

               return xr::ReferenceSpace::create(m_Session, create_info) >> m_ReferenceSpace;
           };
}

toolkit::result<> titan::GraphicsSystem::CreatePipelineLayout()
{
    const std::array push_constant_ranges
    {
        VkPushConstantRange
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(detail::ShaderData),
        },
    };

    const VkPipelineLayoutCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };

    return vk::PipelineLayout::create(m_Device, create_info) >> m_PipelineLayout;
}

toolkit::result<> titan::GraphicsSystem::FillBuffers()
{
    for (auto &[id, info] : m_Meshes)
    {
        auto data = m_Application.GetResources().Get<pkg::mesh::Data>(id);

        info.VertexBufferOffset = 0;
        info.VertexBufferSize = data.GetVertexCount() * sizeof(pkg::mesh::Vertex);
        info.VertexBufferStride = sizeof(pkg::mesh::Vertex);

        info.IndexBufferOffset = 0;
        info.IndexBufferSize = data.GetIndexCount() * sizeof(uint32_t);
        info.IndexType = VK_INDEX_TYPE_UINT32;
        info.IndexCount = data.GetIndexCount();

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = info.VertexMemory,
                .offset = info.VertexBufferOffset,
                .size = info.VertexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = info.VertexMemory,
            };

            void *ptr;
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr; !res)
                return res;

            std::memcpy(ptr, data.GetVertexData(), info.VertexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info); !res)
                return res;
        }

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = info.IndexMemory,
                .offset = info.IndexBufferOffset,
                .size = info.IndexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = info.IndexMemory,
            };

            void *ptr;
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr; !res)
                return res;

            std::memcpy(ptr, data.GetIndexData(), info.IndexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info); !res)
                return res;
        }
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::CreateRenderPass()
{
    const std::array attachments
    {
        VkAttachmentDescription2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .format = m_ColorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        VkAttachmentDescription2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .format = m_DepthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
    };

    const std::array color_attachments
    {
        VkAttachmentReference2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        }
    };

    const VkAttachmentReference2 depth_attachment
    {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    };

    const std::array subpasses
    {
        VkSubpassDescription2
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .viewMask = 0,
            .colorAttachmentCount = color_attachments.size(),
            .pColorAttachments = color_attachments.data(),
            .pDepthStencilAttachment = &depth_attachment,
        }
    };

    const std::array dependencies
    {
        VkSubpassDependency2
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
            .viewOffset = 0,
        }
    };

    const VkRenderPassCreateInfo2 create_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = subpasses.size(),
        .pSubpasses = subpasses.data(),
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data(),
    };

    return vk::RenderPass::create(m_Device, create_info) >> m_RenderPass;
}

toolkit::result<> titan::GraphicsSystem::CreateWindowSurface()
{
    return vk::SurfaceKHR::create(m_Instance, m_Application.GetWindow()) >> m_WindowSurface;
}

toolkit::result<> titan::GraphicsSystem::CreatePipeline()
{
    vk::ShaderModule module_vertex, module_fragment;

    ResourceID vert_id, frag_id;

    if (auto res = m_Application.GetResources().Load("/shader/vert") >> vert_id; !res)
        return res;

    if (auto res = m_Application.GetResources().Load("/shader/frag") >> frag_id; !res)
        return res;

    auto vert_shader = m_Application.GetResources().Get<pkg::shader::Data>(vert_id);
    auto frag_shader = m_Application.GetResources().Get<pkg::shader::Data>(frag_id);

    const VkShaderModuleCreateInfo module_vertex_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vert_shader.GetBinarySize(),
        .pCode = static_cast<const uint32_t *>(vert_shader.GetBinaryData()),
    };

    const VkShaderModuleCreateInfo module_fragment_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = frag_shader.GetBinarySize(),
        .pCode = static_cast<const uint32_t *>(frag_shader.GetBinaryData()),
    };

    if (auto res = vk::ShaderModule::create(m_Device, module_vertex_create_info) >> module_vertex; !res)
        return res;
    if (auto res = vk::ShaderModule::create(m_Device, module_fragment_create_info) >> module_fragment; !res)
        return res;

    const std::array stage_create_info
    {
        VkPipelineShaderStageCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = module_vertex,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = module_fragment,
            .pName = "main",
        },
    };

    const std::array vertex_binding_descriptions
    {
        VkVertexInputBindingDescription
        {
            .binding = 0,
            .stride = sizeof(pkg::mesh::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
    };

    const std::array vertex_attribute_descriptions
    {
        VkVertexInputAttributeDescription
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(pkg::mesh::Vertex, Position),
        },
        VkVertexInputAttributeDescription
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(pkg::mesh::Vertex, Normal),
        },
        VkVertexInputAttributeDescription
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(pkg::mesh::Vertex, Texture),
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertex_binding_descriptions.size(),
        .pVertexBindingDescriptions = vertex_binding_descriptions.data(),
        .vertexAttributeDescriptionCount = vertex_attribute_descriptions.size(),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    const VkPipelineTessellationStateCreateInfo tessellation_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    const std::array attachments
    {
        VkPipelineColorBlendAttachmentState
        {
            .blendEnable = false,
            .srcColorBlendFactor = {},
            .dstColorBlendFactor = {},
            .colorBlendOp = {},
            .srcAlphaBlendFactor = {},
            .dstAlphaBlendFactor = {},
            .alphaBlendOp = {},
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                              | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT
                              | VK_COLOR_COMPONENT_A_BIT,
        },
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = false,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .blendConstants = {
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        },
    };

    const std::array dynamic_states
    {
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };

    const VkGraphicsPipelineCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = stage_create_info.size(),
        .pStages = stage_create_info.data(),
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pTessellationState = &tessellation_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = m_PipelineLayout,
        .renderPass = m_RenderPass,
        .subpass = 0,
    };

    return vk::GraphicsPipeline::create(m_Device, m_PipelineCache, create_info) >> m_Pipeline;
}

static const VkSurfaceFormatKHR &find_surface_format(
    const std::vector<VkSurfaceFormat2KHR> &formats,
    const VkFormat request)
{
    for (auto &format : formats)
        if (format.surfaceFormat.format == request
            && format.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format.surfaceFormat;
    return formats[0].surfaceFormat;
}

static VkPresentModeKHR select_present_mode(const std::vector<VkPresentModeKHR> &modes)
{
    for (auto &mode : modes)
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D select_extent(const VkSurfaceCapabilitiesKHR &capabilities, const titan::glfw::Window &window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()
        && capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    window.GetFramebufferSize(width, height);

    VkExtent2D extent
    {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return extent;
}

toolkit::result<> titan::GraphicsSystem::CreateWindowSwapchainView()
{
    const VkPhysicalDeviceSurfaceInfo2KHR surface_info
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
        .surface = m_WindowSurface,
    };

    VkSurfaceCapabilities2KHR surface_capabilities;
    if (auto res = vk::GetPhysicalDeviceSurfaceCapabilities2KHR(m_PhysicalDevice, surface_info) >> surface_capabilities;
        !res)
        return res;

    auto &capabilities = surface_capabilities.surfaceCapabilities;

    std::vector<VkSurfaceFormat2KHR> surface_formats;
    if (auto res = vk::GetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, surface_info) >> surface_formats; !res)
        return res;

    std::vector<VkPresentModeKHR> present_modes;
    if (auto res = vk::GetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_WindowSurface) >> present_modes; !
        res)
        return res;

    const auto &[color_format, color_space] = find_surface_format(surface_formats, m_ColorFormat);
    const auto presentMode = select_present_mode(present_modes);
    const auto image_extent = select_extent(capabilities, m_Application.GetWindow());

    VkSharingMode image_sharing_mode;
    std::vector<uint32_t> queue_family_indices;

    if (m_QueueFamilyIndices.Default != m_QueueFamilyIndices.Present)
    {
        image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
        queue_family_indices = {
            m_QueueFamilyIndices.Default,
            m_QueueFamilyIndices.Present,
        };
    }
    else
    {
        image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    auto image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    m_WindowSwapchainView = {
        .Width = image_extent.width,
        .Height = image_extent.height,
    };

    {
        const detail::VkSwapchainReferenceCreateInfo create_info
        {
            .useSwapchain = true,
            .imageCount = image_count,
            .imageExtent = image_extent,
            .imageFormat = color_format,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageColorSpace = color_space,
            .imageSharingMode = image_sharing_mode,
            .presentMode = presentMode,
            .preTransform = capabilities.currentTransform,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
        };

        if (auto res = CreateSwapchainReference(create_info) >> m_WindowSwapchainView.Color; !res)
            return res;
    }

    {
        const detail::VkSwapchainReferenceCreateInfo create_info
        {
            .useSwapchain = false,
            .imageCount = image_count,
            .imageExtent = image_extent,
            .imageFormat = m_DepthFormat,
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageSharingMode = image_sharing_mode,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
        };

        if (auto res = CreateSwapchainReference(create_info) >> m_WindowSwapchainView.Depth; !res)
            return res;
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::CreateSynchronization()
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

    if (auto res = vk::Fence::create(m_Device, fence_create_info) >> m_Fence; !res)
        return res;

    for (auto &[
             available,
             finished,
             fence,
             framebuffer,
             buffer
         ] : m_Frames)
    {
        if (auto res = vk::Semaphore::create(m_Device, semaphore_create_info) >> available; !res)
            return res;
        if (auto res = vk::Semaphore::create(m_Device, semaphore_create_info) >> finished; !res)
            return res;
        if (auto res = vk::Fence::create(m_Device, fence_create_info) >> fence; !res)
            return res;
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::CreateBuffers()
{
    for (auto [entity, mesh] : m_Application.GetEntities().Query<component::Mesh>())
    {
        if (m_Meshes.contains(mesh.Resource))
            continue;

        auto &reference = m_Meshes[mesh.Resource];
        auto data = m_Application.GetResources().Get<pkg::mesh::Data>(mesh.Resource);

        reference.BoxMin = data.GetBoxMin();
        reference.BoxMax = data.GetBoxMax();

        reference.BoxCen = reference.BoxMin + 0.5f * (reference.BoxMax - reference.BoxMin);

        {
            const VkBufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = data.GetVertexCount() * sizeof(pkg::mesh::Vertex),
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            if (auto res = vk::Buffer::create(m_Device, create_info) >> reference.VertexBuffer; !res)
                return res;

            const VkBufferMemoryRequirementsInfo2 requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
                .buffer = reference.VertexBuffer,
            };

            const auto requirements = vk::GetBufferMemoryRequirements2(
                        m_Device,
                        requirements_info)
                    .memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                           >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> reference.VertexMemory; !res)
                return res;

            const VkBindBufferMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .buffer = reference.VertexBuffer,
                .memory = reference.VertexMemory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindBufferMemory2(m_Device, bind_info); !res)
                return res;
        }

        {
            const VkBufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = data.GetIndexCount() * sizeof(uint32_t),
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            if (auto res = vk::Buffer::create(m_Device, create_info) >> reference.IndexBuffer; !res)
                return res;

            const VkBufferMemoryRequirementsInfo2 requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
                .buffer = reference.IndexBuffer,
            };

            const auto requirements = vk::GetBufferMemoryRequirements2(
                        m_Device,
                        requirements_info)
                    .memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                           >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> reference.IndexMemory; !res)
                return res;

            const VkBindBufferMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .buffer = reference.IndexBuffer,
                .memory = reference.IndexMemory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindBufferMemory2(m_Device, bind_info); !res)
                return res;
        }
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::CreateSwapchainViews()
{
    VkSharingMode image_sharing_mode;
    std::vector<uint32_t> queue_family_indices;

    if (m_QueueFamilyIndices.Default != m_QueueFamilyIndices.Present)
    {
        image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
        queue_family_indices = {
            m_QueueFamilyIndices.Default,
            m_QueueFamilyIndices.Present,
        };
    }
    else
    {
        image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    auto view_configuration_views = m_Application.GetXrViewConfigurationViews();

    auto view_count = view_configuration_views.size();

    m_SwapchainViews.resize(view_count);

    const VkCommandBufferAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_DefaultPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(view_count),
    };

    std::vector<vk::CommandBuffer> buffers;
    if (auto res = vk::CommandBuffer::create_collection(m_Device, allocate_info) >> buffers; !res)
        return res;

    for (size_t view_index = 0; view_index < view_count; ++view_index)
    {
        auto &[
            view_configuration_view,
            color,
            depth,
            framebuffers,
            buffer
        ] = m_SwapchainViews[view_index];

        view_configuration_view = std::move(view_configuration_views[view_index]);

        const detail::XrSwapchainReferenceCreateInfo color_create_info
        {
            .useSwapchain = true,
            .sampleCount = view_configuration_view.recommendedSwapchainSampleCount,
            .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
            .imageExtent = {
                .width = view_configuration_view.recommendedImageRectWidth,
                .height = view_configuration_view.recommendedImageRectHeight,
            },
            .imageFormat = m_ColorFormat,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        };

        if (auto res = CreateSwapchainReference(color_create_info) >> color; !res)
            return res;

        const detail::XrSwapchainReferenceCreateInfo depth_create_info
        {
            .useSwapchain = false,
            .imageCount = static_cast<uint32_t>(color.Images.size()),
            .imageExtent = {
                .width = view_configuration_view.recommendedImageRectWidth,
                .height = view_configuration_view.recommendedImageRectHeight,
            },
            .imageFormat = m_DepthFormat,
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageSharingMode = image_sharing_mode,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
        };

        if (auto res = CreateSwapchainReference(depth_create_info) >> depth; !res)
            return res;

        const auto framebuffer_count = color.Views.size();

        framebuffers.resize(framebuffer_count);

        for (size_t framebuffer_index = 0; framebuffer_index < framebuffer_count; ++framebuffer_index)
        {
            auto &framebuffer = framebuffers[framebuffer_index];

            const std::vector<VkImageView> attachments
            {
                color.Views[framebuffer_index],
                depth.Views[framebuffer_index],
            };

            const VkFramebufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_RenderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = view_configuration_view.recommendedImageRectWidth,
                .height = view_configuration_view.recommendedImageRectHeight,
                .layers = 1,
            };

            if (auto res = vk::Framebuffer::create(m_Device, create_info) >> framebuffer; !res)
                return res;
        }

        buffer = std::move(buffers[view_index]);
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::CreateFrames()
{
    auto &[
        width,
        height,
        color,
        depth
    ] = m_WindowSwapchainView;

    const auto frame_count = color.Views.size();
    m_Frames.resize(frame_count);

    const VkCommandBufferAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_TransferPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(frame_count),
    };

    std::vector<vk::CommandBuffer> buffers;
    if (auto res = vk::CommandBuffer::create_collection(m_Device, allocate_info) >> buffers; !res)
        return res;

    for (size_t frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        auto &[
            available,
            finished,
            fence,
            framebuffer,
            buffer
        ] = m_Frames[frame_index];

        buffer = std::move(buffers[frame_index]);

        const std::vector<VkImageView> attachments
        {
            color.Views[frame_index],
            depth.Views[frame_index],
        };

        const VkFramebufferCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_RenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = width,
            .height = height,
            .layers = 1,
        };

        if (auto res = vk::Framebuffer::create(m_Device, create_info) >> framebuffer; !res)
            return res;
    }

    return {};
}

#define XR_TO_VK_VERSION(VERSION) VK_MAKE_VERSION(XR_VERSION_MAJOR(VERSION), XR_VERSION_MINOR(VERSION), XR_VERSION_PATCH(VERSION))

toolkit::result<> titan::GraphicsSystem::CreateInstance()
{
    return toolkit::result()
           & [&]
           {
               return xr::GetVulkanGraphicsRequirements2KHR(
                   m_Application.GetXrInstance(),
                   m_Application.GetXrSystemId());
           }
           & [&](XrGraphicsRequirementsVulkan2KHR &&graphics_requirements)
           {
               std::vector<const char *> extensions;
               GetInstanceExtensions(extensions);

               const VkApplicationInfo application_info
               {
                   .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                   .pApplicationName = m_Application.GetName().c_str(),
                   .applicationVersion = VK_MAKE_VERSION(
                       m_Application.GetVersion().Major,
                       m_Application.GetVersion().Minor,
                       m_Application.GetVersion().Patch),
                   .pEngineName = "Titan Core",
                   .engineVersion = VK_MAKE_VERSION(
                       m_Application.GetEngineVersion().Major,
                       m_Application.GetEngineVersion().Minor,
                       m_Application.GetEngineVersion().Patch),
                   .apiVersion = XR_TO_VK_VERSION(graphics_requirements.maxApiVersionSupported),
               };

               const VkInstanceCreateInfo instance_create_info
               {
                   .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                   .pApplicationInfo = &application_info,
                   .enabledLayerCount = VK_INSTANCE_LAYERS.size(),
                   .ppEnabledLayerNames = VK_INSTANCE_LAYERS.data(),
                   .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                   .ppEnabledExtensionNames = extensions.data(),
               };

               const XrVulkanInstanceCreateInfoKHR vulkan_instance_create_info
               {
                   .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
                   .systemId = m_Application.GetXrSystemId(),
                   .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
                   .vulkanCreateInfo = &instance_create_info,
               };

               return xr::CreateVulkanInstanceKHR(
                          m_Application.GetXrInstance(),
                          vulkan_instance_create_info) >> m_Instance;
           };
}

toolkit::result<> titan::GraphicsSystem::CreateMessenger()
{
    const VkDebugUtilsMessengerCreateInfoEXT create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = Application::VkDebugCallback,
        .pUserData = this,
    };

    return vk::DebugUtilsMessengerEXT::create(m_Instance, create_info) >> m_Messenger;
}

toolkit::result<titan::detail::VkSwapchainReference> titan::GraphicsSystem::CreateSwapchainReference(
    const detail::VkSwapchainReferenceCreateInfo &create_info)
{
    detail::VkSwapchainReference reference
    {
        .Format = create_info.imageFormat,
    };

    if (create_info.useSwapchain)
    {
        auto set_images = [&reference](const std::vector<VkImage> &images) -> toolkit::result<>
        {
            reference.Images.resize(images.size());
            for (uint32_t i = 0; i < images.size(); ++i)
                reference.Images[i] = vk::Image::wrap(images[i]);

            return {};
        };

        const VkSwapchainCreateInfoKHR swapchain_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_WindowSurface,
            .minImageCount = create_info.imageCount,
            .imageFormat = create_info.imageFormat,
            .imageColorSpace = create_info.imageColorSpace,
            .imageExtent = create_info.imageExtent,
            .imageArrayLayers = 1,
            .imageUsage = create_info.imageUsage,
            .imageSharingMode = create_info.imageSharingMode,
            .queueFamilyIndexCount = create_info.queueFamilyIndexCount,
            .pQueueFamilyIndices = create_info.pQueueFamilyIndices,
            .preTransform = create_info.preTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = create_info.presentMode,
            .clipped = true,
        };

        if (auto res = vk::SwapchainKHR::create(m_Device, swapchain_create_info) >> reference.Swapchain; !res)
            return res;

        if (auto res = vk::GetSwapchainImagesKHR(m_Device, reference.Swapchain) & set_images; !res)
            return res;
    }
    else
    {
        reference.Images.resize(create_info.imageCount);
        reference.Memory.resize(create_info.imageCount);

        for (uint32_t i = 0; i < create_info.imageCount; ++i)
        {
            auto &image = reference.Images[i];
            auto &memory = reference.Memory[i];

            const VkImageCreateInfo image_create_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = create_info.imageFormat,
                .extent = {
                    .width = create_info.imageExtent.width,
                    .height = create_info.imageExtent.height,
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = create_info.imageUsage,
                .sharingMode = create_info.imageSharingMode,
                .queueFamilyIndexCount = create_info.queueFamilyIndexCount,
                .pQueueFamilyIndices = create_info.pQueueFamilyIndices,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            if (auto res = vk::Image::create(m_Device, image_create_info) >> image; !res)
                return res;

            const VkImageMemoryRequirementsInfo2 memory_requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                .image = image,
            };

            VkMemoryRequirements2 memory_requirements2
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
            };
            vkGetImageMemoryRequirements2(m_Device, &memory_requirements_info, &memory_requirements2);

            auto &memory_requirements = memory_requirements2.memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               memory_requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> memory; !res)
                return res;

            const VkBindImageMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                .image = image,
                .memory = memory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindImageMemory2(m_Device, bind_info); !res)
                return res;
        }
    }

    reference.Views.resize(reference.Images.size());

    for (uint32_t i = 0; i < reference.Images.size(); ++i)
    {
        const VkImageViewCreateInfo view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = reference.Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = create_info.aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        if (auto res = vk::ImageView::create(m_Device, view_create_info) >> reference.Views[i]; !res)
            return res;
    }

    return reference;
}

toolkit::result<titan::detail::XrSwapchainReference> titan::GraphicsSystem::CreateSwapchainReference(
    const detail::XrSwapchainReferenceCreateInfo &create_info)
{
    detail::XrSwapchainReference reference
    {
        .Format = create_info.imageFormat,
    };

    if (create_info.useSwapchain)
    {
        const XrSwapchainCreateInfo swapchain_create_info
        {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | create_info.usageFlags,
            .format = create_info.imageFormat,
            .sampleCount = create_info.sampleCount,
            .width = create_info.imageExtent.width,
            .height = create_info.imageExtent.height,
            .faceCount = 1,
            .arraySize = 1,
            .mipCount = 1,
        };

        if (auto res = xr::Swapchain::create(m_Session, swapchain_create_info)
                       >> reference.Swapchain; !res)
            return res;

        auto set_images = [&](const std::vector<XrSwapchainImageVulkan2KHR> &images) -> toolkit::result<>
        {
            reference.Images.resize(images.size());
            for (uint32_t i = 0; i < images.size(); ++i)
                reference.Images[i] = vk::Image::wrap(images[i].image);

            return {};
        };

        if (auto res = xr::EnumerateSwapchainImages<XrSwapchainImageVulkan2KHR>(
                           reference.Swapchain,
                           { .type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR }) & set_images; !res)
            return res;
    }
    else
    {
        reference.Images.resize(create_info.imageCount);
        reference.Memory.resize(create_info.imageCount);

        for (uint32_t i = 0; i < create_info.imageCount; ++i)
        {
            auto &image = reference.Images[i];
            auto &memory = reference.Memory[i];

            const VkImageCreateInfo image_create_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = create_info.imageFormat,
                .extent = {
                    .width = create_info.imageExtent.width,
                    .height = create_info.imageExtent.height,
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = create_info.imageUsage,
                .sharingMode = create_info.imageSharingMode,
                .queueFamilyIndexCount = create_info.queueFamilyIndexCount,
                .pQueueFamilyIndices = create_info.pQueueFamilyIndices,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            if (auto res = vk::Image::create(m_Device, image_create_info) >> image; !res)
                return res;

            const VkImageMemoryRequirementsInfo2 memory_requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                .image = image,
            };

            VkMemoryRequirements2 memory_requirements2
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
            };
            vkGetImageMemoryRequirements2(m_Device, &memory_requirements_info, &memory_requirements2);

            auto &memory_requirements = memory_requirements2.memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               memory_requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> memory; !res)
                return res;

            const VkBindImageMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                .image = image,
                .memory = memory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindImageMemory2(m_Device, bind_info); !res)
                return res;
        }
    }

    reference.Views.resize(reference.Images.size());

    for (uint32_t i = 0; i < reference.Images.size(); ++i)
    {
        const VkImageViewCreateInfo view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = reference.Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = create_info.aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        if (auto res = vk::ImageView::create(m_Device, view_create_info) >> reference.Views[i]; !res)
            return res;
    }

    return reference;
}

toolkit::result<> titan::GraphicsSystem::RenderWindowView()
{
    auto &[
        available,
        finished,
        fence,
        framebuffer,
        buffer
    ] = m_Frames[m_FrameIndex];
    m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();

    const std::vector<VkFence> fences
    {
        fence,
    };

    if (auto res = vkWaitForFences(
        m_Device,
        fences.size(),
        fences.data(),
        true,
        std::numeric_limits<uint64_t>::max()))
        return toolkit::make_error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return toolkit::make_error("vkResetFences => {}", res);

    const VkAcquireNextImageInfoKHR acquire_info
    {
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .swapchain = m_WindowSwapchainView.Color.Swapchain,
        .timeout = std::numeric_limits<uint64_t>::max(),
        .semaphore = available,
        .fence = nullptr,
        .deviceMask = 1,
    };

    uint32_t image_index;
    if (auto res = vkAcquireNextImage2KHR(m_Device, &acquire_info, &image_index))
        return toolkit::make_error("vkAcquireNextImage2KHR => {}", res);

    int width, height;
    m_Application.GetWindow().GetFramebufferSize(width, height);

    glm::mat4 view_matrix;
    {
        auto [orientation, position] = m_Application.GetInputs().GetHead();

        const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
        const auto translation = glm::translate(glm::mat4(1.0f), -position);

        view_matrix = rotation * translation;
    }

    glm::mat4 projection_matrix;
    {
        projection_matrix = glm::perspectiveFovRH_ZO(
            glm::radians(FOV),
            static_cast<float>(width),
            static_cast<float>(height),
            NEAR,
            FAR);

        projection_matrix[1][1] *= -1.0f;
    }

    if (auto res = RecordCommandBuffer(
        {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .screen_matrix = projection_matrix * view_matrix,
            .buffer = buffer,
            .framebuffer = framebuffer
        }); !res)
        return res;

    {
        const std::array wait_semaphores
        {
            VkSemaphoreSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = available,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            },
        };

        const std::array command_buffers
        {
            VkCommandBufferSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = buffer,
            }
        };

        const std::array signal_semaphores
        {
            VkSemaphoreSubmitInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = finished,
                .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            },
        };

        const std::array submits
        {
            VkSubmitInfo2
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = wait_semaphores.size(),
                .pWaitSemaphoreInfos = wait_semaphores.data(),
                .commandBufferInfoCount = command_buffers.size(),
                .pCommandBufferInfos = command_buffers.data(),
                .signalSemaphoreInfoCount = signal_semaphores.size(),
                .pSignalSemaphoreInfos = signal_semaphores.data(),
            },
        };

        if (auto res = vkQueueSubmit2(m_DefaultQueue, submits.size(), submits.data(), fence))
            return toolkit::make_error("vkQueueSubmit2 => {}", res);
    }

    {
        const std::vector<VkSemaphore> wait_semaphores
        {
            finished,
        };

        const std::vector<VkSwapchainKHR> swapchains
        {
            m_WindowSwapchainView.Color.Swapchain,
        };

        const VkPresentInfoKHR present_info
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
            .pWaitSemaphores = wait_semaphores.data(),
            .swapchainCount = static_cast<uint32_t>(swapchains.size()),
            .pSwapchains = swapchains.data(),
            .pImageIndices = &image_index,
        };

        if (auto res = vkQueuePresentKHR(m_PresentQueue, &present_info))
            return toolkit::make_error("vkQueuePresentKHR =>{}", res);
    }

    return {};
}

toolkit::result<> titan::GraphicsSystem::RenderViews(detail::LayerReference &reference)
{
    auto &projection_views = reference.Views;

    const XrViewLocateInfo view_locate_info
    {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = m_Application.GetXrViewConfigurationType(),
        .displayTime = reference.PredictedDisplayTime,
        .space = m_ReferenceSpace,
    };

    XrViewState view_state
    {
        .type = XR_TYPE_VIEW_STATE,
    };

    std::vector<XrView> views;
    if (auto res = xr::LocateViews(m_Session, view_locate_info, view_state) >> views; !res)
        return res;

    projection_views = {
        views.size(),
        { .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW },
    };

    std::vector<VkCommandBufferSubmitInfo> command_buffers
    {
        views.size(),
        { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO },
    };

    const std::vector<VkFence> fences
    {
        m_Fence,
    };

    if (auto res = vkWaitForFences(
        m_Device,
        fences.size(),
        fences.data(),
        true,
        std::numeric_limits<uint64_t>::max()))
        return toolkit::make_error("vkWaitForFences => {}", res);

    if (auto res = vkResetFences(
        m_Device,
        fences.size(),
        fences.data()))
        return toolkit::make_error("vkResetFences => {}", res);

    const XrSwapchainImageAcquireInfo acquire_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
    };

    const XrSwapchainImageReleaseInfo release_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    const XrSwapchainImageWaitInfo wait_info
    {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
        .timeout = XR_INFINITE_DURATION,
    };

    for (uint32_t view_index = 0; view_index < views.size(); ++view_index)
    {
        const auto &view = views[view_index];

        auto &[
            view_configuration_view,
            color,
            depth,
            framebuffers,
            buffer
        ] = m_SwapchainViews[view_index];

        uint32_t image_index;
        if (auto res = xrAcquireSwapchainImage(color.Swapchain, &acquire_info, &image_index))
            return toolkit::make_error("xrAcquireSwapchainImage => {}", res);

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &wait_info))
            return toolkit::make_error("xrWaitSwapchainImage => {}", res);

        const auto width = view_configuration_view.recommendedImageRectWidth;
        const auto height = view_configuration_view.recommendedImageRectHeight;

        projection_views[view_index] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .pose = view.pose,
            .fov = view.fov,
            .subImage = {
                .swapchain = color.Swapchain,
                .imageRect = {
                    .offset = {
                        .x = 0,
                        .y = 0,
                    },
                    .extent = {
                        .width = static_cast<int32_t>(width),
                        .height = static_cast<int32_t>(height),
                    },
                },
                .imageArrayIndex = 0,
            },
        };

        command_buffers[view_index] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = buffer,
        };

        glm::mat4 view_matrix;
        {
            const glm::quat orientation
            {
                view.pose.orientation.w,
                view.pose.orientation.x,
                view.pose.orientation.y,
                view.pose.orientation.z,
            };

            const glm::vec3 position
            {
                view.pose.position.x,
                view.pose.position.y,
                view.pose.position.z,
            };

            const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
            const auto translation = glm::translate(glm::mat4(1.0f), -position);

            view_matrix = rotation * translation;
        }

        glm::mat4 projection_matrix;
        {
            auto l = NEAR * tanf(view.fov.angleLeft);
            auto r = NEAR * tanf(view.fov.angleRight);
            auto b = NEAR * tanf(view.fov.angleDown);
            auto t = NEAR * tanf(view.fov.angleUp);

            projection_matrix = glm::frustumRH_ZO(l, r, t, b, NEAR, FAR);
        }

        if (auto res = RecordCommandBuffer(
            {
                .width = width,
                .height = height,
                .screen_matrix = projection_matrix * view_matrix,
                .buffer = buffer,
                .framebuffer = framebuffers[image_index],
            }); !res)
            return res;
    }

    const std::array submits
    {
        VkSubmitInfo2
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = static_cast<uint32_t>(command_buffers.size()),
            .pCommandBufferInfos = command_buffers.data(),
        },
    };

    if (auto res = vkQueueSubmit2(m_DefaultQueue, submits.size(), submits.data(), m_Fence))
        return toolkit::make_error("vkQueueSubmit2 => {}", res);

    for (auto &view : m_SwapchainViews)
        if (auto res = xrReleaseSwapchainImage(view.Color.Swapchain, &release_info))
            return toolkit::make_error("xrReleaseSwapchainImage => {}", res);

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(projection_views.size()),
        .views = projection_views.data(),
    };

    return {};
}

toolkit::result<> titan::GraphicsSystem::RecordCommandBuffer(const detail::RecordCommandBufferInfo &info)
{
    if (auto res = vk::ResetCommandBuffer(info.buffer, 0); !res)
        return res;

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (auto res = vk::BeginCommandBuffer(info.buffer, command_buffer_begin_info); !res)
        return res;

    const VkViewport viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(info.width),
        .height = static_cast<float>(info.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor
    {
        .offset = {
            .x = 0,
            .y = 0,
        },
        .extent = {
            .width = info.width,
            .height = info.height,
        },
    };

    const std::array clear_values
    {
        VkClearValue{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
        VkClearValue{ .depthStencil = { .depth = 1.0f } },
    };

    const VkRenderPassBeginInfo render_pass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_RenderPass,
        .framebuffer = info.framebuffer,
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = info.width,
                .height = info.height,
            },
        },
        .clearValueCount = clear_values.size(),
        .pClearValues = clear_values.data(),
    };

    const VkSubpassBeginInfo subpass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
        .contents = VK_SUBPASS_CONTENTS_INLINE,
    };

    vkCmdBeginRenderPass2(info.buffer, &render_pass_begin_info, &subpass_begin_info);

    vkCmdBindPipeline(info.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdSetViewportWithCount(info.buffer, 1, &viewport);
    vkCmdSetScissorWithCount(info.buffer, 1, &scissor);

    for (auto [entity, transform, mesh] : m_Application.GetEntities().Query<component::Transform, component::Mesh>())
    {
        if (!entity.Active)
            continue;

        auto &mesh_data = m_Meshes[mesh.Resource];

        const std::array<VkBuffer, 1> buffers
        {
            mesh_data.VertexBuffer,
        };

        const std::array offsets
        {
            mesh_data.VertexBufferOffset,
        };

        vkCmdBindVertexBuffers(
            info.buffer,
            0,
            buffers.size(),
            buffers.data(),
            offsets.data());
        vkCmdBindIndexBuffer(
            info.buffer,
            mesh_data.IndexBuffer,
            mesh_data.IndexBufferOffset,
            mesh_data.IndexType);

        const detail::ShaderData shader_data
        {
            .Screen = info.screen_matrix,
            .Model = transform.Matrix,
            .Normal = glm::transpose(transform.Inverse),
        };

        const VkPushConstantsInfo push_constants_info
        {
            .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
            .layout = m_PipelineLayout,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(detail::ShaderData),
            .pValues = &shader_data,
        };

        vkCmdPushConstants2(info.buffer, &push_constants_info);

        vkCmdDrawIndexed(info.buffer, mesh_data.IndexCount, 1, 0, 0, 0);
    }

    const VkSubpassEndInfo subpass_end_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
    };

    vkCmdEndRenderPass2(info.buffer, &subpass_end_info);

    return vk::EndCommandBuffer(info.buffer);
}
