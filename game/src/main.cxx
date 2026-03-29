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
    void OnStart() override
    {
    }

    void PreFrame() override
    {
    }

    void OnFrame() override
    {
    }

    void PostFrame() override
    {
    }

    void OnStop() override
    {
    }
};

static Game game;

static void signal_handler(const int signal)
{
    static const std::map<int, const char *> signal_map
    {
        { SIGINT, "SIGINT" },
        { SIGTERM, "SIGTERM" },
    };

    game.Terminate();

    std::cerr << "exit on signal " << signal_map.at(signal) << std::endl;
}

int main(const int argc, const char *const *argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = game.Initialize(*argv, { argv + 1, argv + argc })
               | []
               {
                   core::result<bool> e;
                   do
                       e = game.Spin();
                   while (e && *e);
                   return e;
               };

    if (res)
        return 0;

    std::cerr << res.error() << std::endl;
    return 1;
}
