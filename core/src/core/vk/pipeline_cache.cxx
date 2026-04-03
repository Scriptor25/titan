#include <titan/core.hxx>

core::result<> core::Application::CreatePipelineCache()
{
    // TODO: read/write pipeline cache from/to file
    const VkPipelineCacheCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .initialDataSize = 0,
        .pInitialData = nullptr,
    };

    return vk::PipelineCache::create(m_Device, create_info) >> m_PipelineCache;
}
