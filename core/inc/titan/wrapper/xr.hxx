#pragma once

#include <titan/api.hxx>
#include <titan/extension.hxx>
#include <titan/result.hxx>
#include <titan/typename.hxx>
#include <titan/format/xr.hxx>
#include <titan/wrapper/base.hxx>

namespace core
{
    template<>
    constexpr const char *typename_string<XrAction>()
    {
        return "XrAction";
    }

    template<>
    constexpr const char *typename_string<XrActionSet>()
    {
        return "XrActionSet";
    }

    template<>
    constexpr const char *typename_string<XrDebugUtilsMessengerEXT>()
    {
        return "XrDebugUtilsMessengerEXT";
    }

    template<>
    constexpr const char *typename_string<XrEnvironmentBlendMode>()
    {
        return "XrEnvironmentBlendMode";
    }

    template<>
    constexpr const char *typename_string<XrInstance>()
    {
        return "XrInstance";
    }

    template<>
    constexpr const char *typename_string<XrSession>()
    {
        return "XrSession";
    }

    template<>
    constexpr const char *typename_string<XrSpace>()
    {
        return "XrSpace";
    }

    template<>
    constexpr const char *typename_string<XrSwapchain>()
    {
        return "XrSwapchain";
    }

    template<>
    constexpr const char *typename_string<XrSwapchainImageBaseHeader>()
    {
        return "XrSwapchainImageBaseHeader";
    }

    template<>
    constexpr const char *typename_string<XrSwapchainImageVulkanKHR>()
    {
        return "XrSwapchainImageVulkanKHR";
    }

    template<>
    constexpr const char *typename_string<XrSystemId>()
    {
        return "XrSystemId";
    }

    template<>
    constexpr const char *typename_string<XrViewConfigurationType>()
    {
        return "XrViewConfigurationType";
    }

    template<>
    constexpr const char *typename_string<XrViewConfigurationView>()
    {
        return "XrViewConfigurationView";
    }
}

namespace core::xr
{
    template<
        typename T,
        typename I,
        typename P,
        XrResult(*C)(P, const I *, T *),
        XrResult(*D)(P, T)>
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
            if (auto code = C(param, &create_info, &value))
                return error<wrapper_same_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
            return wrapper_same_param(param, value);
        }

    protected:
        void destroy() override
        {
            D(param, this->value);
        }

        void swap(wrapper_same_param &&other) noexcept override
        {
            std::swap(param, other.param);
        }

        P param{};
    };

    template<typename T, typename I, typename CP, XrResult(*C)(CP, const I *, T *), XrResult(*D)(T)>
    class wrapper_create_param : public wrapper_base<T, wrapper_create_param<T, I, CP, C, D>>
    {
        friend wrapper_base<T, wrapper_create_param>;

        explicit wrapper_create_param(T value)
            : wrapper_base<T, wrapper_create_param>(value)
        {
        }

    public:
        wrapper_create_param() = default;

        static result<wrapper_create_param> create(CP create_param, const I &create_info)
        {
            T value;
            if (auto code = C(create_param, &create_info, &value))
                return error<wrapper_create_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
            return wrapper_create_param(value);
        }

    protected:
        void destroy() override
        {
            D(this->value);
        }
    };

    template<typename T, typename I, XrResult(*C)(const I *, T *), XrResult(*D)(T)>
    class wrapper_void_param : public wrapper_base<T, wrapper_void_param<T, I, C, D>>
    {
        friend wrapper_base<T, wrapper_void_param>;

        explicit wrapper_void_param(T value)
            : wrapper_base<T, wrapper_void_param>(value)
        {
        }

    public:
        wrapper_void_param() = default;

        static result<wrapper_void_param> create(const I &create_info)
        {
            T value;
            if (auto code = C(&create_info, &value))
                return error<wrapper_void_param>(
                    "failed to create {} ({})",
                    typename_string<T>(),
                    code);
            return wrapper_void_param(value);
        }

    protected:
        void destroy() override
        {
            D(this->value);
        }
    };

    template<typename T, typename I, typename P, XrResult(*G)(P, const I *, T *)>
    class wrapper_get : public wrapper_base<T, wrapper_get<T, I, P, G>>
    {
        friend wrapper_base<T, wrapper_get>;

        explicit wrapper_get(T value)
            : wrapper_base<T, wrapper_get>(value)
        {
        }

    public:
        wrapper_get() = default;

        static result<wrapper_get> create(P param, const I &get_info)
        {
            T value;
            if (auto code = G(param, &get_info, &value))
                return error<wrapper_get>(
                    "failed to get {} ({})",
                    typename_string<T>(),
                    code);
            return wrapper_get(value);
        }
    };

    using Action = wrapper_create_param<
        XrAction,
        XrActionCreateInfo,
        XrActionSet,
        xrCreateAction,
        xrDestroyAction>;

    using ActionSet = wrapper_create_param<
        XrActionSet,
        XrActionSetCreateInfo,
        XrInstance,
        xrCreateActionSet,
        xrDestroyActionSet>;

    using ActionSpace = wrapper_create_param<
        XrSpace,
        XrActionSpaceCreateInfo,
        XrSession,
        xrCreateActionSpace,
        xrDestroySpace>;

    using DebugUtilsMessengerEXT = wrapper_same_param<
        XrDebugUtilsMessengerEXT,
        XrDebugUtilsMessengerCreateInfoEXT,
        XrInstance,
        xrCreateDebugUtilsMessengerEXT,
        xrDestroyDebugUtilsMessengerEXT>;

    using Instance = wrapper_void_param<
        XrInstance,
        XrInstanceCreateInfo,
        xrCreateInstance,
        xrDestroyInstance>;

    using ReferenceSpace = wrapper_create_param<
        XrSpace,
        XrReferenceSpaceCreateInfo,
        XrSession,
        xrCreateReferenceSpace,
        xrDestroySpace>;

    using Session = wrapper_create_param<
        XrSession,
        XrSessionCreateInfo,
        XrInstance,
        xrCreateSession,
        xrDestroySession>;

    using Swapchain = wrapper_create_param<
        XrSwapchain,
        XrSwapchainCreateInfo,
        XrSession,
        xrCreateSwapchain,
        xrDestroySwapchain>;
}
