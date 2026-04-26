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
    toolkit::result<> OnStart() override
    {
        return titan::ok();
    }

    toolkit::result<> PreFrame() override
    {
        return titan::ok();
    }

    toolkit::result<> OnFrame() override
    {
        return titan::ok();
    }

    toolkit::result<> PostFrame() override
    {
        return titan::ok();
    }

    toolkit::result<> OnStop() override
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
    static constexpr auto name = "NumberComponent";
    static constexpr auto id = titan::hash64(name);

    NumberComponent() = default;

    NumberComponent(const uint32_t value)
        : value(value)
    {
    }

    NumberComponent(const NumberComponent &) = delete;
    NumberComponent &operator=(const NumberComponent &) = delete;

    NumberComponent(NumberComponent &&other) noexcept
        : value(other.value)
    {
    }

    NumberComponent &operator=(NumberComponent &&other) noexcept
    {
        std::swap(value, other.value);
        return *this;
    }

    uint32_t value{};
};

struct StringComponent
{
    static constexpr auto name = "StringComponent";
    static constexpr auto id = titan::hash64(name);

    StringComponent() = default;

    StringComponent(std::string value)
        : value(std::move(value))
    {
    }

    StringComponent(const StringComponent &) = delete;
    StringComponent &operator=(const StringComponent &) = delete;

    StringComponent(StringComponent &&other) noexcept
        : value(std::move(other.value))
    {
    }

    StringComponent &operator=(StringComponent &&other) noexcept
    {
        std::swap(value, other.value);
        return *this;
    }

    std::string value;
};

int main(const int argc, const char *const *argv)
{
    Game game;
    game_ptr = &game;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = game.Initialize(*argv, { argv + 1, argv + argc })
               & [&]
               {
                   toolkit::result<bool> e;
                   do
                       e = game.Spin();
                   while (e && *e);
                   return e;
               };

    if (auto cleanup_res = game.CleanUp(); !cleanup_res)
        std::cerr << "during cleanup phase: " << cleanup_res.error() << std::endl;

    if (res)
    {
        game_ptr = nullptr;
        return 0;
    }

    std::cerr << res.error() << std::endl;

    game_ptr = nullptr;
    return 1;
}
