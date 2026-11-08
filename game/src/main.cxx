#include <titan/component.hxx>
#include <titan/core.hxx>
#include <titan/system/entity.hxx>

#include <pkg/mesh.hxx>

#include <csignal>
#include <iostream>
#include <map>

static void teapot_script(titan::Application &context, const titan::EntityState entity)
{
    static auto begin = std::chrono::high_resolution_clock::now();
    const auto now = std::chrono::high_resolution_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - begin).count();

    auto [transform] = context.GetEntities().Get<titan::component::Transform>(entity.ID);

    transform.Rotation = glm::rotate(
        glm::quat(),
        delta * glm::radians(20.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    transform.Scale = glm::vec3(0.1f);

    transform.Dirty = true;
}

struct CubeState
{
    static constexpr auto name = "CubeState";
    static constexpr auto id = titan::hash64(name);

    size_t Index{};
};

static void cube_script(titan::Application &context, const titan::EntityState entity)
{
    auto [transform, mesh, state] = context.GetEntities().Get<
        titan::component::Transform,
        titan::component::Mesh,
        CubeState
    >(entity.ID);

    const auto hand = context.GetInputs().GetHand(state.Index);

    const auto active = hand.IsActive;
    entity.Active = active;

    if (!active)
        return;

    const auto mesh_view = context.GetResources().Get<pkg::mesh::Data>(mesh.Resource);

    auto &box_min = mesh_view.GetBoxMin();
    auto &box_max = mesh_view.GetBoxMax();

    const auto pivot = box_min + 0.5f * (box_max - box_min);

    transform.Translation = hand.Position;
    transform.Rotation = hand.Orientation;
    transform.Scale = glm::vec3(0.1f);
    transform.Pivot = pivot;

    transform.Dirty = true;
}

struct ControllerState
{
    static constexpr auto name = "ControllerState";
    static constexpr auto id = titan::hash64(name);

    titan::ResourceID TeapotMesh{}, CubeMesh{};
    titan::EntityID Teapot{};
    std::vector<titan::EntityID> Cubes;
};

static void controller_script(titan::Application &context, const titan::EntityState entity)
{
    const auto [state] = context.GetEntities().Get<ControllerState>(entity.ID);

    const auto teapot_mesh = context.GetResources().Get<pkg::mesh::Data>(state.TeapotMesh);
    const auto cube_mesh = context.GetResources().Get<pkg::mesh::Data>(state.CubeMesh);

    struct collider_t
    {
        glm::vec3 cen;
        float rad;
    };

    auto create_collider = [](
        const glm::vec3 &box_min,
        const glm::vec3 &box_max,
        const titan::component::Transform &transform
    ) -> collider_t
    {
        const auto min_t = transform.Matrix * glm::vec4(box_min, 1.0f);
        const auto max_t = transform.Matrix * glm::vec4(box_max, 1.0f);

        const auto min = glm::vec3(min_t) / min_t.w;
        const auto max = glm::vec3(max_t) / max_t.w;
        const auto cen = min + 0.5f * (max - min);
        const auto rad = glm::distance(min, max) * 0.5f;

        return {
            .cen = cen,
            .rad = rad,
        };
    };

    std::vector<collider_t> colliders;
    std::unordered_map<size_t, size_t> cube_colliders;

    {
        const auto [transform] = context.GetEntities().Get<titan::component::Transform>(state.Teapot);
        colliders.push_back(create_collider(teapot_mesh.GetBoxMin(), teapot_mesh.GetBoxMax(), transform));
    }

    for (size_t i = 0; i < state.Cubes.size(); ++i)
    {
        const auto [transform] = context.GetEntities().Get<titan::component::Transform>(state.Cubes[i]);

        cube_colliders[i] = colliders.size();
        colliders.push_back(create_collider(cube_mesh.GetBoxMin(), cube_mesh.GetBoxMax(), transform));
    }

    for (size_t i = 0; i < state.Cubes.size(); ++i)
    {
        const auto [cube_state] = context.GetEntities().Get<CubeState>(state.Cubes[i]);

        const auto hand = context.GetInputs().GetHand(cube_state.Index);
        if (!hand.IsActive)
            continue;

        float max_haptic{};

        const auto &[cube_cen, cube_rad] = colliders[cube_colliders[i]];
        for (size_t j = 0; j < colliders.size(); ++j)
        {
            if (j == cube_colliders[i])
                continue;

            const auto &[cen, rad] = colliders[j];

            const auto distance = glm::distance(cen, cube_cen);
            const auto distance_min = rad + cube_rad;
            const auto distance_max = 1.2f * distance_min;

            if (distance < distance_min)
            {
                max_haptic = 1.0f;
                break;
            }

            if (distance > distance_max)
                continue;

            auto haptic = std::clamp(
                (distance_max - distance) / (distance_max - distance_min),
                0.0f,
                1.0f);

            max_haptic = std::max(max_haptic, haptic);
        }

        hand.Haptic = max_haptic;
    }
}

class Game final : public titan::Application
{
public:
    Game()
        : Application(
            {
                .Name    = "Titan Game",
                .Version = {
                    .Major = 0,
                    .Minor = 0,
                    .Patch = 0,
                },
            }
        )
    {
    }

protected:
    toolkit::result<> OnInitialize() override
    {
        if (auto res = GetResources().Load("/mesh/teapot") >> m_TeapotMesh; !res)
            return res;

        if (auto res = GetResources().Load("/mesh/cube") >> m_CubeMesh; !res)
            return res;

        {
            auto [entity, active] = GetEntities().Create(
                titan::component::Transform{},
                titan::component::Mesh{ m_TeapotMesh },
                titan::component::Script{ teapot_script }
            );

            m_Teapot = entity;
            active = true;
        }

        {
            auto [entity, active] = GetEntities().Create(
                titan::component::Transform{},
                titan::component::Mesh{ m_CubeMesh },
                titan::component::Script{ cube_script },
                CubeState{ 0ull }
            );

            m_CubeL = entity;
            active = false;
        }

        {
            auto [entity, active] = GetEntities().Create(
                titan::component::Transform{},
                titan::component::Mesh{ m_CubeMesh },
                titan::component::Script{ cube_script },
                CubeState{ 1ull }
            );

            m_CubeR = entity;
            active = false;
        }

        {
            auto [entity, active] = GetEntities().Create(
                titan::component::Script{ controller_script },
                ControllerState
                {
                    .TeapotMesh = m_TeapotMesh,
                    .CubeMesh   = m_CubeMesh,
                    .Teapot     = m_Teapot,
                    .Cubes      = { m_CubeL, m_CubeR },
                }
            );

            m_Controller = entity;
            active = true;
        }

        return {};
    }

private:
    titan::ResourceID m_TeapotMesh{}, m_CubeMesh{};
    titan::EntityID m_Teapot{}, m_CubeL{}, m_CubeR{}, m_Controller{};
};

static Game *game_ptr = nullptr;

static void signal_handler(const int signal)
{
    static const std::map<int, const char *> signal_map
    {
        { SIGINT, "SIGINT" },
        { SIGTERM, "SIGTERM" },
    };

    if (game_ptr)
        game_ptr->Terminate();

    std::cerr << "exit on signal " << signal_map.at(signal) << std::endl;
}

int main(const int argc, const char *const *argv)
{
    Game game;
    game_ptr = &game;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = game.Initialize(*argv, { argv + 1, argv + argc })
               & [&]() -> toolkit::result<int>
               {
                   bool value;
                   do
                       if (auto spin = game.Spin() >> value; !spin)
                           return spin;
                   while (value);
                   return { 0 };
               }
               | [](std::string &&error) -> toolkit::result<int>
               {
                   std::cerr << error << std::endl;
                   return { 1 };
               };

    (void) (game.Destroy()
            | [](std::string &&error)
            {
                std::cerr << error << std::endl;
                return toolkit::result();
            });

    game_ptr = nullptr;
    return res.value();
}
