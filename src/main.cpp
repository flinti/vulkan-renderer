#include <string>
#include <set>
#include "../third-party/spdlog/include/spdlog/common.h"
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include "../third-party/spdlog/include/spdlog/sinks/stdout_color_sinks.h"

#include "Application.h"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);

    std::set<std::string> options;

    for (size_t i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            options.insert(argv[i]);
        }
    }
    if (options.size()) {
        spdlog::info("command line options:");
        for (auto &o : options) {
            spdlog::info("\n\t'{}'", o);
        }
    }

    bool singleFrame = false;
    if (options.find("--single") != options.end()) {
        singleFrame = true;
    }

#ifdef NDEBUG
    spdlog::info("Release build.");
#else
    spdlog::info("Debug build");
#endif

    try {
        Application app(true, 3, singleFrame);
        app.run();
    } catch (const std::exception& e) {
        spdlog::critical("Exception {}: {}", typeid(e).name(), e.what());
        return 1;
    }

    return 0;
}
