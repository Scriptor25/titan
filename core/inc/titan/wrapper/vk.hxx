#pragma once

#include <titan/api.hxx>
#include <titan/heap.hxx>
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
            Heap *heap,
            VkDevice device,
            const VkBufferCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkBufferCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateBuffer(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyBuffer(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkCommandBuffer>
    {
        using value_type = VkCommandBuffer;

        static constexpr auto create_name = "vkAllocateCommandBuffers";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info)
        {
            return std::tuple{ heap, device, allocate_info.commandPool };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info,
            value_type &value)
        {
            if (const auto result = vkAllocateCommandBuffers(device, &allocate_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, pool = allocate_info.commandPool, value]
                {
                    destroy(heap, device, pool, value);
                },
                device,
                allocate_info.commandPool);

            return VK_SUCCESS;
        }

        static auto create_collection(
            Heap *heap,
            VkDevice device,
            const VkCommandBufferAllocateInfo &allocate_info,
            std::vector<value_type> &values)
        {
            values.resize(allocate_info.commandBufferCount);

            if (const auto result = vkAllocateCommandBuffers(device, &allocate_info, values.data()))
                return result;

            for (const auto value : values)
                heap->Insert(
                    value,
                    [heap, device, pool = allocate_info.commandPool, value]
                    {
                        destroy(heap, device, pool, value);
                    },
                    device,
                    allocate_info.commandPool);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            VkCommandPool pool,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkFreeCommandBuffers(device, pool, 1, &value);

            heap->Erase(value);
        }

        static void destroy_collection(
            Heap *heap,
            VkDevice device,
            VkCommandPool pool,
            const std::vector<value_type> &values)
        {
            for (const auto value : values)
                if (heap->Uses(value))
                    return;

            vkFreeCommandBuffers(device, pool, values.size(), values.data());

            for (const auto value : values)
                heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkCommandPool>
    {
        using value_type = VkCommandPool;

        static constexpr auto create_name = "vkCreateCommandPool";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkCommandPoolCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkCommandPoolCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateCommandPool(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyCommandPool(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkDebugUtilsMessengerEXT>
    {
        using value_type = VkDebugUtilsMessengerEXT;

        static constexpr auto create_name = "vkCreateDebugUtilsMessengerEXT";

        static auto make_destroy_args(
            Heap *heap,
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT &)
        {
            return std::tuple{ heap, instance };
        }

        static auto create(
            Heap *heap,
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, instance, value]
                {
                    destroy(heap, instance, value);
                },
                instance);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkInstance instance,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyDebugUtilsMessengerEXT(instance, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkDescriptorPool>
    {
        using value_type = VkDescriptorPool;

        static constexpr auto create_name = "vkCreateDescriptorPool";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkDescriptorPoolCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkDescriptorPoolCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateDescriptorPool(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyDescriptorPool(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkDescriptorSet>
    {
        using value_type = VkDescriptorSet;

        static constexpr auto create_name = "vkAllocateDescriptorSets";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info)
        {
            return std::tuple{ heap, device, allocate_info.descriptorPool };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info,
            value_type &value)
        {
            if (const auto result = vkAllocateDescriptorSets(device, &allocate_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, pool = allocate_info.descriptorPool, value]
                {
                    destroy(heap, device, pool, value);
                },
                device,
                allocate_info.descriptorPool);

            return VK_SUCCESS;
        }

        static auto create_collection(
            Heap *heap,
            VkDevice device,
            const VkDescriptorSetAllocateInfo &allocate_info,
            std::vector<value_type> &values)
        {
            values.resize(allocate_info.descriptorSetCount);
            if (const auto result = vkAllocateDescriptorSets(device, &allocate_info, values.data()))
                return result;

            for (const auto value : values)
            {
                heap->Insert(
                    value,
                    [heap, device, pool = allocate_info.descriptorPool, value]
                    {
                        destroy(heap, device, pool, value);
                    },
                    device,
                    allocate_info.descriptorPool);
            }

            return VK_SUCCESS;
        }

        static VkResult destroy(
            Heap *heap,
            VkDevice device,
            VkDescriptorPool pool,
            value_type value)
        {
            if (heap->Uses(value))
                return VK_SUCCESS;

            if (const auto result = vkFreeDescriptorSets(device, pool, 1, &value))
                return result;

            heap->Erase(value);

            return VK_SUCCESS;
        }

        static VkResult destroy_collection(
            Heap *heap,
            VkDevice device,
            VkDescriptorPool pool,
            const std::vector<value_type> &values)
        {
            for (const auto value : values)
                if (heap->Uses(value))
                    return VK_SUCCESS;

            if (const auto result = vkFreeDescriptorSets(device, pool, values.size(), values.data()))
                return result;

            for (const auto value : values)
                heap->Erase(value);

            return VK_SUCCESS;
        }
    };

    template<>
    struct traits_t<VkDescriptorSetLayout>
    {
        using value_type = VkDescriptorSetLayout;

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkDescriptorSetLayoutCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkDescriptorSetLayoutCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyDescriptorSetLayout(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkDevice>
    {
        using value_type = VkDevice;

        static auto make_destroy_args(
            Heap *heap,
            VkPhysicalDevice physical_device,
            const VkDeviceCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static auto create(
            Heap *heap,
            VkPhysicalDevice physical_device,
            const VkDeviceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateDevice(physical_device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                physical_device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyDevice(value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkDeviceMemory>
    {
        using value_type = VkDeviceMemory;

        static constexpr auto create_name = "vkAllocateMemory";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkMemoryAllocateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkMemoryAllocateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkAllocateMemory(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkFreeMemory(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkFence>
    {
        using value_type = VkFence;

        static constexpr auto create_name = "vkCreateFence";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkFenceCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkFenceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateFence(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyFence(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkFramebuffer>
    {
        using value_type = VkFramebuffer;

        static constexpr auto create_name = "vkCreateFramebuffer";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkFramebufferCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkFramebufferCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateFramebuffer(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device,
                create_info.renderPass);

            heap->Use(value, std::span(create_info.pAttachments, create_info.attachmentCount));

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyFramebuffer(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkPipeline, VkGraphicsPipelineCreateInfo>
    {
        using value_type = VkPipeline;

        static constexpr auto create_name = "vkCreateGraphicsPipelines";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            VkPipelineCache,
            const VkGraphicsPipelineCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            VkPipelineCache cache,
            const VkGraphicsPipelineCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateGraphicsPipelines(device, cache, 1, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device,
                cache,
                create_info.renderPass,
                create_info.layout);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyPipeline(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkImage>
    {
        using value_type = VkImage;

        static constexpr auto create_name = "vkCreateImage";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkImageCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkImageCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateImage(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyImage(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkImageView>
    {
        using value_type = VkImageView;

        static constexpr auto create_name = "vkCreateImageView";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkImageViewCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkImageViewCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateImageView(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device,
                create_info.image);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyImageView(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkInstance>
    {
        using value_type = VkInstance;

        static auto make_destroy_args(
            Heap *heap,
            const VkInstanceCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static auto create(
            Heap *heap,
            const VkInstanceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateInstance(&create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                });

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyInstance(value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkPhysicalDevice>
    {
        using value_type = VkPhysicalDevice;

        static auto make_destroy_args(Heap *heap)
        {
            return std::tuple{ heap };
        }

        static void destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkPipelineCache>
    {
        using value_type = VkPipelineCache;

        static constexpr auto create_name = "vkCreatePipelineCache";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkPipelineCacheCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkPipelineCacheCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreatePipelineCache(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyPipelineCache(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkPipelineLayout>
    {
        using value_type = VkPipelineLayout;

        static constexpr auto create_name = "vkCreatePipelineLayout";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkPipelineLayoutCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkPipelineLayoutCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreatePipelineLayout(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);
            heap->Use(value, std::span(create_info.pSetLayouts, create_info.setLayoutCount));

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyPipelineLayout(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkQueue>
    {
        using value_type = VkQueue;

        static auto make_destroy_args(Heap *heap)
        {
            return std::tuple{ heap };
        }

        static void destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkRenderPass>
    {
        using value_type = VkRenderPass;

        static constexpr auto create_name = "vkCreateRenderPass";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkRenderPassCreateInfo2 &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkRenderPassCreateInfo2 &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateRenderPass2(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyRenderPass(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkSemaphore>
    {
        using value_type = VkSemaphore;

        static constexpr auto create_name = "vkCreateSemaphore";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkSemaphoreCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkSemaphoreCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateSemaphore(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroySemaphore(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkShaderModule>
    {
        using value_type = VkShaderModule;

        static constexpr auto create_name = "vkCreateShaderModule";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkShaderModuleCreateInfo &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkShaderModuleCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateShaderModule(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroyShaderModule(device, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkSurfaceKHR>
    {
        using value_type = VkSurfaceKHR;

        static constexpr auto create_name = "glfwCreateWindowSurfaceKHR";

        static auto make_destroy_args(
            Heap *heap,
            VkInstance instance,
            GLFWwindow *)
        {
            return std::tuple{ heap, instance };
        }

        static auto create(
            Heap *heap,
            VkInstance instance,
            GLFWwindow *window,
            value_type &value)
        {
            if (const auto result = glfwCreateWindowSurface(instance, window, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, instance, value]
                {
                    destroy(heap, instance, value);
                },
                instance,
                window);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkInstance instance,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroySurfaceKHR(instance, value, nullptr);

            heap->Erase(value);
        }
    };

    template<>
    struct traits_t<VkSwapchainKHR>
    {
        using value_type = VkSwapchainKHR;

        static constexpr auto create_name = "vkCreateSwapchainKHR";

        static auto make_destroy_args(
            Heap *heap,
            VkDevice device,
            const VkSwapchainCreateInfoKHR &)
        {
            return std::tuple{ heap, device };
        }

        static auto create(
            Heap *heap,
            VkDevice device,
            const VkSwapchainCreateInfoKHR &create_info,
            value_type &value)
        {
            if (const auto result = vkCreateSwapchainKHR(device, &create_info, nullptr, &value))
                return result;

            heap->Insert(
                value,
                [heap, device, value]
                {
                    destroy(heap, device, value);
                },
                device,
                create_info.surface);

            return VK_SUCCESS;
        }

        static void destroy(
            Heap *heap,
            VkDevice device,
            value_type value)
        {
            if (heap->Uses(value))
                return;

            vkDestroySwapchainKHR(device, value, nullptr);

            heap->Erase(value);
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
        using PhysicalDevice = wrapper_t<VkPhysicalDevice>;
        using PipelineCache = wrapper_t<VkPipelineCache>;
        using PipelineLayout = wrapper_t<VkPipelineLayout>;
        using Queue = wrapper_t<VkQueue>;
        using RenderPass = wrapper_t<VkRenderPass>;
        using Semaphore = wrapper_t<VkSemaphore>;
        using ShaderModule = wrapper_t<VkShaderModule>;
        using SurfaceKHR = wrapper_t<VkSurfaceKHR>;
        using SwapchainKHR = wrapper_t<VkSwapchainKHR>;
    }
}
