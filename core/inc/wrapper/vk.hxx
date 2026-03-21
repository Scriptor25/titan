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
            if (auto code = C(create_param, &create_info, nullptr, &value))
                return error<wrapper>("failed to create {} ({})", typename_string<T>(), code);
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
            if (auto code = C(param, &create_info, nullptr, &value))
                return error<wrapper_same_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
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
            if (auto code = C(create_param, &create_info, nullptr, &value))
                return error<wrapper_create_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
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
            if (auto code = C(&create_info, nullptr, &value))
                return error<wrapper_destroy_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
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
            if (auto code = C(&create_info, nullptr, &value))
                return error<wrapper_void_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
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
            if (auto code = glfwCreateWindowSurface(instance, window, nullptr, &value))
                return error<GLFWSurface>(
                    "failed to create {} ({})",
                    typename_string<VkSurfaceKHR>(),
                    code);
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
}
