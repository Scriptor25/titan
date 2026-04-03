#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<> core::Application::CreatePipelineCache()
{
    auto data = LoadBinary("pipeline-cache");

    const VkPipelineCacheCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .initialDataSize = data.size(),
        .pInitialData = data.data(),
    };

    return vk::PipelineCache::create(m_Device, create_info) >> m_PipelineCache;
}

core::result<> core::Application::StorePipelineCache()
{
    return vk::GetPipelineCacheData(m_Device, m_PipelineCache)
           & [&](std::vector<char> &&data)
           {
               StoreBinary("pipeline-cache", data);
               return ok();
           };
}
