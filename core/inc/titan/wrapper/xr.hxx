#pragma once

#include <titan/api.hxx>
#include <titan/extension.hxx>
#include <titan/format/xr.hxx>
#include <titan/wrapper/base.hxx>

namespace titan
{
    template<>
    struct traits_t<XrAction>
    {
        using value_type = XrAction;

        static constexpr auto create_name = "xrCreateAction";

        static auto make_destroy_args(XrActionSet, const XrActionCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrActionSet action_set,
            const XrActionCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateAction(action_set, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroyAction(value);
        }
    };

    template<>
    struct traits_t<XrActionSet>
    {
        using value_type = XrActionSet;

        static constexpr auto create_name = "xrCreateActionSet";

        static auto make_destroy_args(XrInstance, const XrActionSetCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrInstance instance,
            const XrActionSetCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateActionSet(instance, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroyActionSet(value);
        }
    };

    template<>
    struct traits_t<XrDebugUtilsMessengerEXT>
    {
        using value_type = XrDebugUtilsMessengerEXT;

        static constexpr auto create_name = "xrCreateDebugUtilsMessengerEXT";

        static auto make_destroy_args(
            XrInstance instance,
            const XrDebugUtilsMessengerCreateInfoEXT &)
        {
            return std::tuple{ instance };
        }

        static auto create(
            XrInstance instance,
            const XrDebugUtilsMessengerCreateInfoEXT &create_info,
            value_type &value)
        {
            return xrCreateDebugUtilsMessengerEXT(instance, &create_info, &value);
        }

        static auto destroy(
            XrInstance instance,
            value_type value)
        {
            return xrDestroyDebugUtilsMessengerEXT(instance, value);
        }
    };

    template<>
    struct traits_t<XrInstance>
    {
        using value_type = XrInstance;

        static constexpr auto create_name = "xrCreateInstance";

        static auto make_destroy_args(const XrInstanceCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            const XrInstanceCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateInstance(&create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroyInstance(value);
        }
    };

    template<>
    struct traits_t<XrSession>
    {
        using value_type = XrSession;

        static constexpr auto create_name = "xrCreateSession";

        static auto make_destroy_args(
            XrInstance,
            const XrSessionCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrInstance instance,
            const XrSessionCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateSession(instance, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroySession(value);
        }
    };

    template<>
    struct traits_t<XrSpace, XrActionSpaceCreateInfo>
    {
        using value_type = XrSpace;

        static constexpr auto create_name = "xrCreateActionSpace";

        static auto make_destroy_args(
            XrSession,
            const XrActionSpaceCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrSession session,
            const XrActionSpaceCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateActionSpace(session, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroySpace(value);
        }
    };

    template<>
    struct traits_t<XrSpace, XrReferenceSpaceCreateInfo>
    {
        using value_type = XrSpace;

        static constexpr auto create_name = "xrCreateReferenceSpace";

        static auto make_destroy_args(
            XrSession,
            const XrReferenceSpaceCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrSession session,
            const XrReferenceSpaceCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateReferenceSpace(session, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroySpace(value);
        }
    };

    template<>
    struct traits_t<XrSwapchain>
    {
        using value_type = XrSwapchain;

        static constexpr auto create_name = "xrCreateSwapchain";

        static auto make_destroy_args(
            XrSession,
            const XrSwapchainCreateInfo &)
        {
            return std::tuple{};
        }

        static auto create(
            XrSession session,
            const XrSwapchainCreateInfo &create_info,
            value_type &value)
        {
            return xrCreateSwapchain(session, &create_info, &value);
        }

        static auto destroy(value_type value)
        {
            return xrDestroySwapchain(value);
        }
    };

    namespace xr
    {
        using Action = wrapper_t<XrAction>;
        using ActionSet = wrapper_t<XrActionSet>;
        using ActionSpace = wrapper_t<XrSpace, XrActionSpaceCreateInfo>;
        using DebugUtilsMessengerEXT = wrapper_t<XrDebugUtilsMessengerEXT>;
        using Instance = wrapper_t<XrInstance>;
        using ReferenceSpace = wrapper_t<XrSpace, XrReferenceSpaceCreateInfo>;
        using Session = wrapper_t<XrSession>;
        using Swapchain = wrapper_t<XrSwapchain>;
    }
}
