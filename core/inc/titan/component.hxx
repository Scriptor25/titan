#pragma once

#include <titan/hash.hxx>
#include <titan/system/resource.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace titan
{
    struct Context
    {
        float Aspect = 1.0f;
    };

    struct BaseComponent
    {
        virtual ~BaseComponent() = default;

        virtual void Update(const Context &context);
    };

    struct TransformComponent : BaseComponent
    {
        static constexpr auto name = "TransformComponent";
        static constexpr auto id = hash64(name);

        TransformComponent() = default;

        void Update(const Context &context) override;

        glm::vec3 Position{ 0.0f };
        glm::quat Orientation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f };
        glm::vec3 Pivot{ 0.0f };

        glm::mat4 Matrix{ 1.0f }, Inverse{ 1.0f };
    };

    struct FrustumCameraComponent : BaseComponent
    {
        static constexpr auto name = "FrustumCameraComponent";
        static constexpr auto id = hash64(name);

        FrustumCameraComponent() = default;

        void Update(const Context &context) override;

        float Left = -1.0f, Right = 1.0f, Bottom = -1.0f, Top = 1.0f, Near = 0.01f, Far = 100.0f;

        glm::mat4 Matrix{ 1.0f }, Inverse{ 1.0f };
    };

    struct AngleFrustumCameraComponent : BaseComponent
    {
        static constexpr auto name = "AngleFrustumCameraComponent";
        static constexpr auto id = hash64(name);

        AngleFrustumCameraComponent() = default;

        void Update(const Context &context) override;

        float FovLeft = 0.0f, FovRight = 0.0f, FovDown = 0.0f, FovUp = 0.0f, Near = 0.01f, Far = 100.0f;

        glm::mat4 Matrix{ 1.0f }, Inverse{ 1.0f };
    };

    struct PerspectiveCameraComponent : BaseComponent
    {
        static constexpr auto name = "PerspectiveCameraComponent";
        static constexpr auto id = hash64(name);

        PerspectiveCameraComponent() = default;

        void Update(const Context &context) override;

        float FovY = 90.0f, Near = 0.01f, Far = 100.0f;

        glm::mat4 Matrix{ 1.0f }, Inverse{ 1.0f };
    };

    struct MeshComponent : BaseComponent
    {
        static constexpr auto name = "MeshComponent";
        static constexpr auto id = hash64(name);

        MeshComponent() = default;

        ResourceID Resource{};
    };

    struct MaterialComponent : BaseComponent
    {
        static constexpr auto name = "MaterialComponent";
        static constexpr auto id = hash64(name);

        MaterialComponent() = default;

        ResourceID Resource{};
    };
}
