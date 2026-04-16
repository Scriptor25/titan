#pragma once

#include <titan/api.hxx>
#include <titan/format/vk.hxx>
#include <titan/wrapper/base.hxx>

#include <vector>

namespace titan
{
    template<>
    struct traits_t<VkBuffer>
    {
        using value_type = VkBuffer;

        static constexpr auto create_name = "vkCreateBuffer";

        static auto make_destroy_args(
            VkDevice device,
            const VkBufferCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkBufferCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateBuffer(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyBuffer(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkCommandBuffer>
    {
        using value_type = VkCommandBuffer;

        static constexpr auto create_name = "vkAllocateCommandBuffers";

        static auto make_destroy_args(
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info)
        {
            return std::tuple{ device, allocate_info.commandPool };
        }

        static auto create(
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info,
            value_type &value)
        {
            return vkAllocateCommandBuffers(device, &allocate_info, &value);
        }

        static auto create_collection(
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info,
            std::vector<value_type> &values)
        {
            values.resize(allocate_info.commandBufferCount);
            return vkAllocateCommandBuffers(device, &allocate_info, values.data());
        }

        static auto destroy(
            VkDevice device,
            VkCommandPool pool,
            value_type value)
        {
            return vkFreeCommandBuffers(device, pool, 1, &value);
        }

        static auto destroy_collection(
            VkDevice device,
            VkCommandPool pool,
            const std::vector<value_type> &values)
        {
            return vkFreeCommandBuffers(device, pool, values.size(), values.data());
        }
    };

    template<>
    struct traits_t<VkCommandPool>
    {
        using value_type = VkCommandPool;

        static constexpr auto create_name = "vkCreateCommandPool";

        static auto make_destroy_args(
            VkDevice device,
            const VkCommandPoolCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkCommandPoolCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateCommandPool(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyCommandPool(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkDebugUtilsMessengerEXT>
    {
        using value_type = VkDebugUtilsMessengerEXT;

        static constexpr auto create_name = "vkCreateDebugUtilsMessengerEXT";

        static auto make_destroy_args(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT &)
        {
            return std::tuple{ instance };
        }

        static auto create(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT &create_info,
            value_type &value)
        {
            return vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkInstance instance,
            value_type value)
        {
            return vkDestroyDebugUtilsMessengerEXT(instance, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkDescriptorPool>
    {
        using value_type = VkDescriptorPool;

        static constexpr auto create_name = "vkCreateDescriptorPool";

        static auto make_destroy_args(
            VkDevice device,
            const VkDescriptorPoolCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkDescriptorPoolCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateDescriptorPool(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyDescriptorPool(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkDescriptorSet>
    {
        using value_type = VkDescriptorSet;

        static constexpr auto create_name = "vkAllocateDescriptorSets";

        static auto make_destroy_args(
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info)
        {
            return std::tuple{ device, allocate_info.descriptorPool };
        }

        static auto create(
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info,
            value_type &value)
        {
            return vkAllocateDescriptorSets(device, &allocate_info, &value);
        }

        static auto create_collection(
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info,
            std::vector<value_type> &values)
        {
            values.resize(allocate_info.descriptorSetCount);
            return vkAllocateDescriptorSets(device, &allocate_info, values.data());
        }

        static auto destroy(
            VkDevice device,
            VkDescriptorPool pool,
            value_type value)
        {
            return vkFreeDescriptorSets(device, pool, 1, &value);
        }

        static auto destroy_collection(
            VkDevice device,
            VkDescriptorPool pool,
            const std::vector<value_type> &values)
        {
            return vkFreeDescriptorSets(device, pool, values.size(), values.data());
        }
    };

    template<>
    struct traits_t<VkDescriptorSetLayout>
    {
        using value_type = VkDescriptorSetLayout;

        static auto make_destroy_args(
            VkDevice device,
            const VkDescriptorSetLayoutCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkDescriptorSetLayoutCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateDescriptorSetLayout(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyDescriptorSetLayout(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkDevice>
    {
        using value_type = VkDevice;

        static auto make_destroy_args(
            VkPhysicalDevice,
            const VkDeviceCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            VkPhysicalDevice physical_device,
            const VkDeviceCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateDevice(physical_device, &create_info, nullptr, &value);
        }

        static auto destroy(value_type value)
        {
            return vkDestroyDevice(value, nullptr);
        }
    };

    template<>
    struct traits_t<VkDeviceMemory>
    {
        using value_type = VkDeviceMemory;

        static constexpr auto create_name = "vkAllocateMemory";

        static auto make_destroy_args(
            VkDevice device,
            const VkMemoryAllocateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkMemoryAllocateInfo &create_info,
            value_type &value)
        {
            return vkAllocateMemory(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkFreeMemory(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkFence>
    {
        using value_type = VkFence;

        static constexpr auto create_name = "vkCreateFence";

        static auto make_destroy_args(
            VkDevice device,
            const VkFenceCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkFenceCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateFence(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyFence(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkFramebuffer>
    {
        using value_type = VkFramebuffer;

        static constexpr auto create_name = "vkCreateFramebuffer";

        static auto make_destroy_args(
            VkDevice device,
            const VkFramebufferCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkFramebufferCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateFramebuffer(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyFramebuffer(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkPipeline, VkGraphicsPipelineCreateInfo>
    {
        using value_type = VkPipeline;

        static constexpr auto create_name = "vkCreateGraphicsPipelines";

        static auto make_destroy_args(
            VkDevice device,
            VkPipelineCache,
            const VkGraphicsPipelineCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            VkPipelineCache cache,
            const VkGraphicsPipelineCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateGraphicsPipelines(device, cache, 1, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyPipeline(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkImage>
    {
        using value_type = VkImage;

        static constexpr auto create_name = "vkCreateImage";

        static auto make_destroy_args(
            VkDevice device,
            const VkImageCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkImageCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateImage(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyImage(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkImageView>
    {
        using value_type = VkImageView;

        static constexpr auto create_name = "vkCreateImageView";

        static auto make_destroy_args(
            VkDevice device,
            const VkImageViewCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkImageViewCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateImageView(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyImageView(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkInstance>
    {
        using value_type = VkInstance;

        static auto make_destroy_args(const VkInstanceCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            const VkInstanceCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateInstance(&create_info, nullptr, &value);
        }

        static auto destroy(value_type value)
        {
            return vkDestroyInstance(value, nullptr);
        }
    };

    template<>
    struct traits_t<VkPipelineCache>
    {
        using value_type = VkPipelineCache;

        static constexpr auto create_name = "vkCreatePipelineCache";

        static auto make_destroy_args(
            VkDevice device,
            const VkPipelineCacheCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkPipelineCacheCreateInfo &create_info,
            value_type &value)
        {
            return vkCreatePipelineCache(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyPipelineCache(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkPipelineLayout>
    {
        using value_type = VkPipelineLayout;

        static constexpr auto create_name = "vkCreatePipelineLayout";

        static auto make_destroy_args(
            VkDevice device,
            const VkPipelineLayoutCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkPipelineLayoutCreateInfo &create_info,
            value_type &value)
        {
            return vkCreatePipelineLayout(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyPipelineLayout(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkRenderPass>
    {
        using value_type = VkRenderPass;

        static constexpr auto create_name = "vkCreateRenderPass";

        static auto make_destroy_args(
            VkDevice device,
            const VkRenderPassCreateInfo2 &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkRenderPassCreateInfo2 &create_info,
            value_type &value)
        {
            return vkCreateRenderPass2(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyRenderPass(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkSemaphore>
    {
        using value_type = VkSemaphore;

        static constexpr auto create_name = "vkCreateSemaphore";

        static auto make_destroy_args(
            VkDevice device,
            const VkSemaphoreCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkSemaphoreCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateSemaphore(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroySemaphore(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkShaderModule>
    {
        using value_type = VkShaderModule;

        static constexpr auto create_name = "vkCreateShaderModule";

        static auto make_destroy_args(
            VkDevice device,
            const VkShaderModuleCreateInfo &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkShaderModuleCreateInfo &create_info,
            value_type &value)
        {
            return vkCreateShaderModule(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroyShaderModule(device, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkSurfaceKHR>
    {
        using value_type = VkSurfaceKHR;

        static constexpr auto create_name = "glfwCreateWindowSurfaceKHR";

        static auto make_destroy_args(
            VkInstance instance,
            GLFWwindow *)
        {
            return std::tuple{ instance };
        }

        static auto create(
            VkInstance instance,
            GLFWwindow *window,
            value_type &value)
        {
            return glfwCreateWindowSurface(instance, window, nullptr, &value);
        }

        static auto destroy(
            VkInstance instance,
            value_type value)
        {
            return vkDestroySurfaceKHR(instance, value, nullptr);
        }
    };

    template<>
    struct traits_t<VkSwapchainKHR>
    {
        using value_type = VkSwapchainKHR;

        static constexpr auto create_name = "vkCreateSwapchainKHR";

        static auto make_destroy_args(
            VkDevice device,
            const VkSwapchainCreateInfoKHR &)
        {
            return std::tuple{ device };
        }

        static auto create(
            VkDevice device,
            const VkSwapchainCreateInfoKHR &create_info,
            value_type &value)
        {
            return vkCreateSwapchainKHR(device, &create_info, nullptr, &value);
        }

        static auto destroy(
            VkDevice device,
            value_type value)
        {
            return vkDestroySwapchainKHR(device, value, nullptr);
        }
    };

    namespace vk
    {
        using Buffer = wrapper_t<VkBuffer>;
        using CommandBuffer = wrapper_t<VkCommandBuffer>;
        using CommandPool = wrapper_t<VkCommandPool>;
        using ComputePipeline = wrapper_t<VkPipeline, VkComputePipelineCreateInfo>;
        using DebugUtilsMessengerEXT = wrapper_t<VkDebugUtilsMessengerEXT>;
        using DescriptorPool = wrapper_t<VkDescriptorPool>;
        using DescriptorSet = wrapper_t<VkDescriptorSet>;
        using DescriptorSetLayout = wrapper_t<VkDescriptorSetLayout>;
        using Device = wrapper_t<VkDevice>;
        using DeviceMemory = wrapper_t<VkDeviceMemory>;
        using Fence = wrapper_t<VkFence>;
        using Framebuffer = wrapper_t<VkFramebuffer>;
        using GraphicsPipeline = wrapper_t<VkPipeline, VkGraphicsPipelineCreateInfo>;
        using Image = wrapper_t<VkImage>;
        using ImageView = wrapper_t<VkImageView>;
        using Instance = wrapper_t<VkInstance>;
        using PipelineCache = wrapper_t<VkPipelineCache>;
        using PipelineLayout = wrapper_t<VkPipelineLayout>;
        using RenderPass = wrapper_t<VkRenderPass>;
        using Semaphore = wrapper_t<VkSemaphore>;
        using ShaderModule = wrapper_t<VkShaderModule>;
        using SurfaceKHR = wrapper_t<VkSurfaceKHR>;
        using SwapchainKHR = wrapper_t<VkSwapchainKHR>;
    }
}
