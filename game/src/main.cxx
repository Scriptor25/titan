#include <titan/core.hxx>

#include <csignal>
#include <iostream>
#include <map>

core::Instance instance;

static void signal_handler(const int signal)
{
    static const std::map<int, const char *> signal_map
    {
        { SIGINT, "SIGINT" },
        { SIGTERM, "SIGTERM" },
    };

    instance.Terminate();

    std::cerr << "exit on signal " << signal_map.at(signal) << std::endl;
}

int main(const int argc, const char *const *argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto res = instance.Initialize(*argv, { argv + 1, argv + argc })
               | []
               {
                   core::result<> e;
                   while ((e = instance.Spin()));
                   return e;
               };

    if (res)
        return 0;

    std::cerr << res.error() << std::endl;
    return 0;
}
