#include <titan/core.hxx>

#include <csignal>
#include <iostream>
#include <map>

class Game final : core::Application
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

protected:
    core::result<> OnStart() override
    {
        return core::ok();
    }

    core::result<> PreFrame() override
    {
        return core::ok();
    }

    core::result<> OnFrame() override
    {
        return core::ok();
    }

    core::result<> PostFrame() override
    {
        return core::ok();
    }

    core::result<> OnStop() override
    {
        return core::ok();
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

int main(const int argc, const char *const *argv)
{
    Game game;
    game_ptr = &game;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = game.Initialize(*argv, { argv + 1, argv + argc })
               & [&]
               {
                   core::result<bool> e;
                   do
                       e = game.Spin();
                   while (!e && *e.value);
                   return e;
               };

    if (!res)
    {
        game_ptr = nullptr;
        return 0;
    }

    std::cerr << res.value.error() << std::endl;

    game_ptr = nullptr;
    return 1;
}
