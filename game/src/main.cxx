#include <titan/core.hxx>
#include <titan/ecs.hxx>

#include <csignal>
#include <iostream>
#include <map>

class Game final : titan::Application
{
public:
    Game()
        : Application(
            {
                .Name = "Titan Game",
                .Version = {
                    .Major = 0,
                    .Minor = 0,
                    .Patch = 0,
                },
            }
        )
    {
    }

    using Application::Initialize;
    using Application::Terminate;
    using Application::Spin;
    using Application::CleanUp;

protected:
    titan::result<> OnStart() override
    {
        return titan::ok();
    }

    titan::result<> PreFrame() override
    {
        return titan::ok();
    }

    titan::result<> OnFrame() override
    {
        return titan::ok();
    }

    titan::result<> PostFrame() override
    {
        return titan::ok();
    }

    titan::result<> OnStop() override
    {
        return titan::ok();
    }
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

template<>
struct titan::component_traits_t<std::string>
{
    static constexpr auto id = 0;
};

template<>
struct titan::component_traits_t<uint32_t>
{
    static constexpr auto id = 1;
};

int main(const int argc, const char *const *argv)
{
    {
        titan::ECS ecs;
        auto hello = ecs.Create<std::string>("Hello");
        auto world = ecs.Create<std::string>("World");
        auto deadbeef = ecs.Create<std::string, uint32_t>("!", 0xDEADBEEFU);

        (void) hello;
        (void) world;
        (void) deadbeef;

        ecs.Add<uint32_t>(world, 0xBADF00DU);

        auto result = ecs.Query<uint32_t>();

        std::apply(
            [](auto &&values)
            {
                for (auto &&value : values)
                    std::cerr << std::hex << value << std::endl;
            },
            result);
    }

    Game game;
    game_ptr = &game;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = game.Initialize(*argv, { argv + 1, argv + argc })
               & [&]
               {
                   titan::result<bool> e;
                   do
                       e = game.Spin();
                   while (!e && *e);
                   return e;
               };

    if (auto cleanup_res = game.CleanUp())
        std::cerr << "during cleanup phase: " << cleanup_res.error() << std::endl;

    if (!res)
    {
        game_ptr = nullptr;
        return 0;
    }

    std::cerr << res.error() << std::endl;

    game_ptr = nullptr;
    return 1;
}
