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
        static constexpr auto destroy_name = "xrDestroyAction";

        static auto make_destroy_args(
            Heap *heap,
            XrActionSet,
            const XrActionCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrActionSet action_set,
            const XrActionCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateAction(action_set, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                action_set);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroyAction(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrActionSet>
    {
        using value_type = XrActionSet;

        static constexpr auto create_name = "xrCreateActionSet";
        static constexpr auto destroy_name = "xrDestroyActionSet";

        static auto make_destroy_args(
            Heap *heap,
            XrInstance,
            const XrActionSetCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrInstance instance,
            const XrActionSetCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateActionSet(instance, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                instance);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroyActionSet(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrDebugUtilsMessengerEXT>
    {
        using value_type = XrDebugUtilsMessengerEXT;

        static constexpr auto create_name = "xrCreateDebugUtilsMessengerEXT";
        static constexpr auto destroy_name = "xrDestroyDebugUtilsMessengerEXT";

        static auto make_destroy_args(
            Heap *heap,
            XrInstance instance,
            const XrDebugUtilsMessengerCreateInfoEXT &)
        {
            return std::tuple{ heap, instance };
        }

        static XrResult create(
            Heap *heap,
            XrInstance instance,
            const XrDebugUtilsMessengerCreateInfoEXT &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateDebugUtilsMessengerEXT(instance, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, instance, value]
                {
                    destroy(heap, instance, value);
                },
                instance);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            XrInstance instance,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroyDebugUtilsMessengerEXT(instance, value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrInstance>
    {
        using value_type = XrInstance;

        static constexpr auto create_name = "xrCreateInstance";
        static constexpr auto destroy_name = "xrDestroyInstance";

        static auto make_destroy_args(
            Heap *heap,
            const XrInstanceCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            const XrInstanceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateInstance(&create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                });

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroyInstance(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrSession>
    {
        using value_type = XrSession;

        static constexpr auto create_name = "xrCreateSession";
        static constexpr auto destroy_name = "xrDestroySession";

        static auto make_destroy_args(
            Heap *heap,
            XrInstance,
            const XrSessionCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrInstance instance,
            const XrSessionCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateSession(instance, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                instance);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroySession(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrSpace, XrActionSpaceCreateInfo>
    {
        using value_type = XrSpace;

        static constexpr auto create_name = "xrCreateActionSpace";
        static constexpr auto destroy_name = "xrDestroySpace<ActionSpace>";

        static auto make_destroy_args(
            Heap *heap,
            XrSession,
            const XrActionSpaceCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrSession session,
            const XrActionSpaceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateActionSpace(session, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                session,
                create_info.action);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroySpace(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrSpace, XrReferenceSpaceCreateInfo>
    {
        using value_type = XrSpace;

        static constexpr auto create_name = "xrCreateReferenceSpace";
        static constexpr auto destroy_name = "xrDestroySpace<ReferenceSpace>";

        static auto make_destroy_args(
            Heap *heap,
            XrSession,
            const XrReferenceSpaceCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrSession session,
            const XrReferenceSpaceCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateReferenceSpace(session, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                session);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroySpace(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
        }
    };

    template<>
    struct traits_t<XrSwapchain>
    {
        using value_type = XrSwapchain;

        static constexpr auto create_name = "xrCreateSwapchain";
        static constexpr auto destroy_name = "xrDestroySwapchain";

        static auto make_destroy_args(
            Heap *heap,
            XrSession,
            const XrSwapchainCreateInfo &)
        {
            return std::tuple{ heap };
        }

        static XrResult create(
            Heap *heap,
            XrSession session,
            const XrSwapchainCreateInfo &create_info,
            value_type &value)
        {
            if (const auto result = xrCreateSwapchain(session, &create_info, &value))
                return result;

            heap->Insert(
                value,
                [heap, value]
                {
                    destroy(heap, value);
                },
                session);

            return XR_SUCCESS;
        }

        static XrResult destroy(
            Heap *heap,
            value_type value)
        {
            if (heap->Uses(value))
                return XR_SUCCESS;

            if (const auto result = xrDestroySwapchain(value))
                return result;

            heap->Erase(value);
            return XR_SUCCESS;
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
