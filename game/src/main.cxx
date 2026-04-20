#include <titan/core.hxx>
#include <titan/hash.hxx>
#include <titan/system/entity.hxx>

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

struct NumberComponent
{
    static constexpr auto id = "NumberComponent"_hash64;

    uint32_t value;
};

struct StringComponent
{
    static constexpr auto id = "StringComponent"_hash64;

    std::string value;
};

int main(const int argc, const char *const *argv)
{
    {
        titan::EntitySystem sys;
        const auto a = sys.Create(StringComponent{ "Hello" });
        const auto b = sys.Create(StringComponent{ "World" });
        const auto c = sys.Create(StringComponent{ "!" }, NumberComponent{ 0xDEADBEEFu });

        (void) a;
        (void) b;
        (void) c;

        sys.Add(b, NumberComponent{ 0xBADF00Du });

        auto &arc = sys.GetArchetype<StringComponent>();
        auto &column = arc.GetColumn(StringComponent::id);
        auto view = column.Cast<StringComponent>();

        for (auto &entry : view)
            std::cerr << entry.value << std::endl;

        for (auto [string, number, same_string] : sys.Query<StringComponent, NumberComponent, StringComponent>())
            std::cerr
                    << string.value
                    << " ( == "
                    << same_string.value
                    << "): "
                    << std::hex
                    << number.value
                    << std::endl;
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
