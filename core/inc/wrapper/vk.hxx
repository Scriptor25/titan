#pragma once

#include <api.hxx>
#include <format.hxx>
#include <result.hxx>
#include <typename.hxx>
#include <wrapper.hxx>
#include <format/vk.hxx>

namespace core
{
    template<>
    constexpr const char *typename_string<VkSurfaceKHR>()
    {
        return "VkSurfaceKHR";
    }

    template<>
    constexpr const char *typename_string<VkPipeline>()
    {
        return "VkPipeline";
    }

    template<>
    constexpr const char *typename_string<VkInstance>()
    {
        return "VkInstance";
    }

    template<>
    constexpr const char *typename_string<VkDevice>()
    {
        return "VkDevice";
    }

    template<>
    constexpr const char *typename_string<VkDebugUtilsMessengerEXT>()
    {
        return "VkDebugUtilsMessengerEXT";
    }

    template<>
    constexpr const char *typename_string<VkImage>()
    {
        return "VkImage";
    }

    template<>
    constexpr const char *typename_string<VkImageView>()
    {
        return "VkImageView";
    }

    template<>
    constexpr const char *typename_string<VkRenderPass>()
    {
        return "VkRenderPass";
    }

    template<>
    constexpr const char *typename_string<VkShaderModule>()
    {
        return "VkShaderModule";
    }

    template<>
    constexpr const char *typename_string<VkPipelineCache>()
    {
        return "VkPipelineCache";
    }

    template<>
    constexpr const char *typename_string<VkPipelineLayout>()
    {
        return "VkPipelineLayout";
    }

    template<>
    constexpr const char *typename_string<VkFramebuffer>()
    {
        return "VkFramebuffer";
    }

    template<>
    constexpr const char *typename_string<VkCommandPool>()
    {
        return "VkCommandPool";
    }

    template<>
    constexpr const char *typename_string<VkCommandBuffer>()
    {
        return "VkCommandBuffer";
    }

    template<>
    constexpr const char *typename_string<VkSemaphore>()
    {
        return "VkSemaphore";
    }

    template<>
    constexpr const char *typename_string<VkFence>()
    {
        return "VkFence";
    }

    template<>
    constexpr const char *typename_string<VkDescriptorSetLayout>()
    {
        return "VkDescriptorSetLayout";
    }

    template<>
    constexpr const char *typename_string<VkDescriptorSet>()
    {
        return "VkDescriptorSet";
    }

    template<>
    constexpr const char *typename_string<VkDescriptorPool>()
    {
        return "VkDescriptorPool";
    }

    template<>
    constexpr const char *typename_string<VkBuffer>()
    {
        return "VkBuffer";
    }

    template<>
    constexpr const char *typename_string<VkDeviceMemory>()
    {
        return "VkDeviceMemory";
    }
}

namespace core::vk
{
    template<
        typename T,
        typename I,
        typename CP,
        typename DP,
        VkResult(*C)(CP, const I *, const VkAllocationCallbacks *, T *),
        void(*D)(DP, T, const VkAllocationCallbacks *)>
    class wrapper : public wrapper_base<T, wrapper<T, I, CP, DP, C, D>>
    {
        explicit wrapper(DP destroy_param, T value)
            : wrapper_base<T, wrapper>(value),
              destroy_param(destroy_param)
        {
        }

    public:
        wrapper() = default;

        static result<wrapper> create(CP create_param, DP destroy_param, const I &create_info)
        {
            T value;
            if (auto res = C(create_param, &create_info, nullptr, &value))
                return error<wrapper>("failed to create {} ({})", typename_string<T>(), res);
            return wrapper(destroy_param, value);
        }

        static wrapper wrap(DP destroy_param, T value)
        {
            return wrapper(destroy_param, value);
        }

    protected:
        void destroy() override
        {
            D(destroy_param, this->value, nullptr);
        }

        void swap(wrapper &&other) noexcept override
        {
            std::swap(destroy_param, other.destroy_param);
        }

        DP destroy_param{};
    };

    template<
        typename T,
        typename I,
        typename P,
        VkResult(*C)(P, const I *, const VkAllocationCallbacks *, T *),
        void(*D)(P, T, const VkAllocationCallbacks *)>
    class wrapper_same_param : public wrapper_base<T, wrapper_same_param<T, I, P, C, D>>
    {
        explicit wrapper_same_param(P param, T value)
            : wrapper_base<T, wrapper_same_param>(value),
              param(param)
        {
        }

    public:
        wrapper_same_param() = default;

        static result<wrapper_same_param> create(P param, const I &create_info)
        {
            T value;
            if (auto res = C(param, &create_info, nullptr, &value))
                return error<wrapper_same_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    res);
            return wrapper_same_param(param, value);
        }

        static wrapper_same_param wrap(P param, T value)
        {
            return wrapper_same_param(param, value);
        }

        static wrapper_same_param wrap(T value)
        {
            return wrapper_same_param({}, value);
        }

    protected:
        void destroy() override
        {
            if (param)
            {
                D(param, this->value, nullptr);
                param = {};
            }
        }

        void swap(wrapper_same_param &&other) noexcept override
        {
            std::swap(param, other.param);
        }

        P param{};
    };

    template<
        typename T,
        typename I,
        typename CP,
        VkResult(*C)(CP, const I *, const VkAllocationCallbacks *, T *),
        void(*D)(T, const VkAllocationCallbacks *)>
    class wrapper_create_param : public wrapper_base<T, wrapper_create_param<T, I, CP, C, D>>
    {
        explicit wrapper_create_param(T value)
            : wrapper_base<T, wrapper_create_param>(value)
        {
        }

    public:
        wrapper_create_param() = default;

        static result<wrapper_create_param> create(CP create_param, const I &create_info)
        {
            T value;
            if (auto res = C(create_param, &create_info, nullptr, &value))
                return error<wrapper_create_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    res);
            return wrapper_create_param(value);
        }

        static wrapper_create_param wrap(T value)
        {
            return wrapper_create_param(value);
        }

    protected:
        void destroy() override
        {
            D(this->value, nullptr);
        }
    };

    template<
        typename T,
        typename I,
        typename DP,
        VkResult(*C)(const I *, const VkAllocationCallbacks *, T *),
        void(*D)(DP, T, const VkAllocationCallbacks *)>
    class wrapper_destroy_param : public wrapper_base<T, wrapper_destroy_param<T, I, DP, C, D>>
    {
        explicit wrapper_destroy_param(DP destroy_param, T value)
            : wrapper_base<T, wrapper_destroy_param>(value),
              destroy_param(destroy_param)
        {
        }

    public:
        wrapper_destroy_param() = default;

        static result<wrapper_destroy_param> create(DP destroy_param, const I &create_info)
        {
            T value;
            if (auto res = C(&create_info, nullptr, &value))
                return error<wrapper_destroy_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    res);
            return wrapper_destroy_param(destroy_param, value);
        }

        static wrapper_destroy_param wrap(DP destroy_param, T value)
        {
            return wrapper_destroy_param(destroy_param, value);
        }

    protected:
        void destroy() override
        {
            D(destroy_param, this->value, nullptr);
        }

        void swap(wrapper_destroy_param &&other) noexcept override
        {
            std::swap(destroy_param, other.destroy_param);
        }

        DP destroy_param{};
    };

    template<
        typename T,
        typename I,
        VkResult(*C)(const I *, const VkAllocationCallbacks *, T *),
        void(*D)(T, const VkAllocationCallbacks *)>
    class wrapper_void_param : public wrapper_base<T, wrapper_void_param<T, I, C, D>>
    {
        explicit wrapper_void_param(T value)
            : wrapper_base<T, wrapper_void_param>(value)
        {
        }

    public:
        wrapper_void_param() = default;

        static result<wrapper_void_param> create(const I &create_info)
        {
            T value;
            if (auto res = C(&create_info, nullptr, &value))
                return error<wrapper_void_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    res);
            return wrapper_void_param(value);
        }

        static wrapper_void_param wrap(T value)
        {
            return wrapper_void_param(value);
        }

    protected:
        void destroy() override
        {
            D(this->value, nullptr);
        }
    };

    class GLFWSurface : public wrapper_base<VkSurfaceKHR, GLFWSurface>
    {
        explicit GLFWSurface(VkInstance instance, VkSurfaceKHR value)
            : wrapper_base(value),
              instance(instance)
        {
        }

    public:
        GLFWSurface() = default;

        static result<GLFWSurface> create(VkInstance instance, GLFWwindow *window)
        {
            VkSurfaceKHR value;
            if (auto res = glfwCreateWindowSurface(instance, window, nullptr, &value))
                return error<GLFWSurface>(
                    "failed to create {} ({})",
                    typename_string<VkSurfaceKHR>(),
                    res);
            return GLFWSurface(instance, value);
        }

        static GLFWSurface wrap(VkInstance instance, VkSurfaceKHR value)
        {
            return GLFWSurface(instance, value);
        }

    protected:
        void destroy() override
        {
            vkDestroySurfaceKHR(instance, value, nullptr);
        }

        void swap(GLFWSurface &&other) noexcept override
        {
            std::swap(instance, other.instance);
        }

        VkInstance instance{};
    };

    class Pipeline : public wrapper_base<VkPipeline, Pipeline>
    {
        explicit Pipeline(VkDevice device, VkPipeline value)
            : wrapper_base(value),
              device(device)
        {
        }

    public:
        Pipeline() = default;

        static result<Pipeline> create(
            VkDevice device,
            VkPipelineCache cache,
            const VkGraphicsPipelineCreateInfo &create_info)
        {
            VkPipeline value;
            if (auto res = vkCreateGraphicsPipelines(device, cache, 1, &create_info, nullptr, &value))
                return error<Pipeline>(
                    "failed to create {} ({})",
                    typename_string<VkPipeline>(),
                    res);
            return Pipeline(device, value);
        }

        static result<std::vector<Pipeline>> create2(
            VkDevice device,
            VkPipelineCache cache,
            const std::vector<VkGraphicsPipelineCreateInfo> &create_infos)
        {
            std::vector<VkPipeline> values(create_infos.size());
            if (auto res = vkCreateGraphicsPipelines(
                device,
                cache,
                create_infos.size(),
                create_infos.data(),
                nullptr,
                values.data()))
                return error<std::vector<Pipeline>>(
                    "failed to create <{} x {}> ({})",
                    values.size(),
                    typename_string<VkPipeline>(),
                    res);

            std::vector<Pipeline> vec(values.size());
            for (uint32_t i = 0; i < values.size(); ++i)
                vec[i] = Pipeline(device, values[i]);
            return vec;
        }

        static result<Pipeline> create(
            VkDevice device,
            VkPipelineCache cache,
            const VkComputePipelineCreateInfo &create_info)
        {
            VkPipeline value;
            if (auto res = vkCreateComputePipelines(device, cache, 1, &create_info, nullptr, &value))
                return error<Pipeline>(
                    "failed to create {} ({})",
                    typename_string<VkPipeline>(),
                    res);
            return Pipeline(device, value);
        }

        static result<std::vector<Pipeline>> create2(
            VkDevice device,
            VkPipelineCache cache,
            const std::vector<VkComputePipelineCreateInfo> &create_infos)
        {
            std::vector<VkPipeline> values(create_infos.size());
            if (auto res = vkCreateComputePipelines(
                device,
                cache,
                create_infos.size(),
                create_infos.data(),
                nullptr,
                values.data()))
                return error<std::vector<Pipeline>>(
                    "failed to create <{} x {}> ({})",
                    values.size(),
                    typename_string<VkPipeline>(),
                    res);

            std::vector<Pipeline> vec(values.size());
            for (uint32_t i = 0; i < values.size(); ++i)
                vec[i] = Pipeline(device, values[i]);
            return vec;
        }

        static Pipeline wrap(VkDevice device, VkPipeline value)
        {
            return Pipeline(device, value);
        }

    protected:
        void destroy() override
        {
            vkDestroyPipeline(device, value, nullptr);
        }

        void swap(Pipeline &&other) noexcept override
        {
            std::swap(device, other.device);
        }

        VkDevice device{};
    };

    class CommandBuffer : public wrapper_base<VkCommandBuffer, CommandBuffer>
    {
        explicit CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer value)
            : wrapper_base(value),
              device(device),
              pool(pool)
        {
        }

    public:
        CommandBuffer() = default;

        static result<CommandBuffer> create(
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info)
        {
            VkCommandBuffer value;
            if (auto res = vkAllocateCommandBuffers(device, &allocate_info, &value))
                return error<CommandBuffer>(
                    "failed to create {} ({})",
                    typename_string<VkCommandBuffer>(),
                    res);
            return CommandBuffer(device, allocate_info.commandPool, value);
        }

        static result<std::vector<CommandBuffer>> create2(
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info)
        {
            std::vector<VkCommandBuffer> values(allocate_info.commandBufferCount);
            if (auto res = vkAllocateCommandBuffers(device, &allocate_info, values.data()))
                return error<std::vector<CommandBuffer>>(
                    "failed to create <{} x {}> ({})",
                    values.size(),
                    typename_string<VkCommandBuffer>(),
                    res);

            std::vector<CommandBuffer> vec(values.size());
            for (uint32_t i = 0; i < values.size(); ++i)
                vec[i] = CommandBuffer(device, allocate_info.commandPool, values[i]);
            return vec;
        }

        static CommandBuffer wrap(VkDevice device, VkCommandPool pool, VkCommandBuffer value)
        {
            return CommandBuffer(device, pool, value);
        }

    protected:
        void destroy() override
        {
            vkFreeCommandBuffers(device, pool, 1, &value);
        }

        void swap(CommandBuffer &&other) noexcept override
        {
            std::swap(device, other.device);
            std::swap(pool, other.pool);
        }

        VkDevice device{};
        VkCommandPool pool{};
    };

    class DescriptorSet : public wrapper_base<VkDescriptorSet, DescriptorSet>
    {
        explicit DescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSet value)
            : wrapper_base(value),
              device(device),
              pool(pool)
        {
        }

    public:
        DescriptorSet() = default;

        static result<DescriptorSet> create(VkDevice device, const VkDescriptorSetAllocateInfo &allocate_info)
        {
            VkDescriptorSet value;
            if (auto res = vkAllocateDescriptorSets(device, &allocate_info, &value))
                return error<DescriptorSet>(
                    "failed to create {} ({})",
                    typename_string<VkDescriptorSet>(),
                    res);
            return DescriptorSet(device, allocate_info.descriptorPool, value);
        }

        static result<std::vector<DescriptorSet>> create2(
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info)
        {
            std::vector<VkDescriptorSet> values(allocate_info.descriptorSetCount);
            if (auto res = vkAllocateDescriptorSets(device, &allocate_info, values.data()))
                return error<std::vector<DescriptorSet>>(
                    "failed to create <{} x {}> ({})",
                    values.size(),
                    typename_string<VkDescriptorSet>(),
                    res);

            std::vector<DescriptorSet> vec(values.size());
            for (uint32_t i = 0; i < values.size(); ++i)
                vec[i] = DescriptorSet(device, allocate_info.descriptorPool, values[i]);
            return vec;
        }

        static DescriptorSet wrap(VkDevice device, VkDescriptorPool pool, VkDescriptorSet value)
        {
            return DescriptorSet(device, pool, value);
        }

    protected:
        void destroy() override
        {
            vkFreeDescriptorSets(device, pool, 1, &value);
        }

        void swap(DescriptorSet &&other) noexcept override
        {
            std::swap(device, other.device);
            std::swap(pool, other.pool);
        }

        VkDevice device{};
        VkDescriptorPool pool{};
    };

    using Instance = wrapper_void_param<
        VkInstance,
        VkInstanceCreateInfo,
        vkCreateInstance,
        vkDestroyInstance>;

    using Device = wrapper_create_param<
        VkDevice,
        VkDeviceCreateInfo,
        VkPhysicalDevice,
        vkCreateDevice,
        vkDestroyDevice>;

    using DebugUtilsMessengerEXT = wrapper_same_param<
        VkDebugUtilsMessengerEXT,
        VkDebugUtilsMessengerCreateInfoEXT,
        VkInstance,
        vkCreateDebugUtilsMessengerEXT,
        vkDestroyDebugUtilsMessengerEXT>;

    using Buffer = wrapper_same_param<
        VkBuffer,
        VkBufferCreateInfo,
        VkDevice,
        vkCreateBuffer,
        vkDestroyBuffer>;

    using Image = wrapper_same_param<
        VkImage,
        VkImageCreateInfo,
        VkDevice,
        vkCreateImage,
        vkDestroyImage>;

    using ImageView = wrapper_same_param<
        VkImageView,
        VkImageViewCreateInfo,
        VkDevice,
        vkCreateImageView,
        vkDestroyImageView>;

    using PipelineCache = wrapper_same_param<
        VkPipelineCache,
        VkPipelineCacheCreateInfo,
        VkDevice,
        vkCreatePipelineCache,
        vkDestroyPipelineCache>;

    using PipelineLayout = wrapper_same_param<
        VkPipelineLayout,
        VkPipelineLayoutCreateInfo,
        VkDevice,
        vkCreatePipelineLayout,
        vkDestroyPipelineLayout>;

    using RenderPass = wrapper_same_param<
        VkRenderPass,
        VkRenderPassCreateInfo,
        VkDevice,
        vkCreateRenderPass,
        vkDestroyRenderPass>;

    using ShaderModule = wrapper_same_param<
        VkShaderModule,
        VkShaderModuleCreateInfo,
        VkDevice,
        vkCreateShaderModule,
        vkDestroyShaderModule>;

    using DescriptorSetLayout = wrapper_same_param<
        VkDescriptorSetLayout,
        VkDescriptorSetLayoutCreateInfo,
        VkDevice,
        vkCreateDescriptorSetLayout,
        vkDestroyDescriptorSetLayout>;

    using Framebuffer = wrapper_same_param<
        VkFramebuffer,
        VkFramebufferCreateInfo,
        VkDevice,
        vkCreateFramebuffer,
        vkDestroyFramebuffer>;

    using CommandPool = wrapper_same_param<
        VkCommandPool,
        VkCommandPoolCreateInfo,
        VkDevice,
        vkCreateCommandPool,
        vkDestroyCommandPool>;

    using Semaphore = wrapper_same_param<
        VkSemaphore,
        VkSemaphoreCreateInfo,
        VkDevice,
        vkCreateSemaphore,
        vkDestroySemaphore>;

    using Fence = wrapper_same_param<
        VkFence,
        VkFenceCreateInfo,
        VkDevice,
        vkCreateFence,
        vkDestroyFence>;

    using DescriptorPool = wrapper_same_param<
        VkDescriptorPool,
        VkDescriptorPoolCreateInfo,
        VkDevice,
        vkCreateDescriptorPool,
        vkDestroyDescriptorPool>;

    using DeviceMemory = wrapper_same_param<
        VkDeviceMemory,
        VkMemoryAllocateInfo,
        VkDevice,
        vkAllocateMemory,
        vkFreeMemory>;
}
