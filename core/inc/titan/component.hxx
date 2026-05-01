#pragma once

#include <titan/hash.hxx>
#include <titan/system/entity.hxx>
#include <titan/system/resource.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace titan
{
    class Application;
}

namespace titan::component
{
    struct Script
    {
        static constexpr auto name = "titan::component::Script";
        static constexpr auto id = hash64(name);

        void (*Callee)(Application &, EntityState){};
    };

    struct Transform
    {
        static constexpr auto name = "titan::component::Transform";
        static constexpr auto id = hash64(name);

        glm::vec3 Translation{ 0.0f };
        glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f };
        glm::vec3 Pivot{ 0.0f };

        bool Dirty = true;
        glm::mat4 Matrix{ 1.0f };
        glm::mat4 Inverse{ 1.0f };
    };

    struct FrustumCamera
    {
        static constexpr auto name = "titan::component::FrustumCamera";
        static constexpr auto id = hash64(name);

        float Left = -1.0f;
        float Right = 1.0f;
        float Bottom = -1.0f;
        float Top = 1.0f;
        float Near = 0.01f;
        float Far = 100.0f;

        bool Dirty = true;
        glm::mat4 Matrix{ 1.0f };
        glm::mat4 Inverse{ 1.0f };
    };

    struct AngleFrustumCamera
    {
        static constexpr auto name = "titan::component::AngleFrustumCamera";
        static constexpr auto id = hash64(name);

        float FovLeft = 0.0f;
        float FovRight = 0.0f;
        float FovDown = 0.0f;
        float FovUp = 0.0f;
        float Near = 0.01f;
        float Far = 100.0f;

        bool Dirty = true;
        glm::mat4 Matrix{ 1.0f };
        glm::mat4 Inverse{ 1.0f };
    };

    struct PerspectiveCamera
    {
        static constexpr auto name = "titan::component::PerspectiveCamera";
        static constexpr auto id = hash64(name);

        float FovY = 90.0f;
        float Near = 0.01f;
        float Far = 100.0f;

        bool Dirty = true;
        glm::mat4 Matrix{ 1.0f };
        glm::mat4 Inverse{ 1.0f };
    };

    struct Mesh
    {
        static constexpr auto name = "titan::component::Mesh";
        static constexpr auto id = hash64(name);

        ResourceID Resource{};
    };

    struct Material
    {
        static constexpr auto name = "titan::component::Material";
        static constexpr auto id = hash64(name);

        ResourceID Resource{};
    };
}
