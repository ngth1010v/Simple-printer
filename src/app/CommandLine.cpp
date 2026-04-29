#include "app/CommandLine.h"

#include <map>
#include <algorithm>
#include <iostream>

namespace CommandLine {

    // -------------------- storage --------------------

    static std::map<std::string, bool> g_flags;
    static std::map<std::string, std::string> g_options;
    static std::vector<std::string> g_args;

    // -------------------- helpers --------------------

    static inline bool StartsWith(const std::string& s, const std::string& prefix) {
        return s.rfind(prefix, 0) == 0;
    }

    static std::string NormalizeKey(const std::string& s) {
        std::string key = s;

        if (StartsWith(key, "--"))
            key = key.substr(2);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        return key;
    }

    // -------------------- parse --------------------

    void Parse(int argc, char** argv) {
        g_flags.clear();
        g_options.clear();
        g_args.clear();

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            // ----- option: --key=value -----
            if (StartsWith(arg, "--") && arg.find('=') != std::string::npos) {
                auto pos = arg.find('=');

                std::string key = NormalizeKey(arg.substr(0, pos));
                std::string value = arg.substr(pos + 1);

                g_options[key] = value;
                continue;
            }

            // ----- flag: --flag -----
            if (StartsWith(arg, "--")) {
                std::string key = NormalizeKey(arg);
                g_flags[key] = true;
                continue;
            }

            // ----- arg -----
            g_args.push_back(arg);
        }
    }

    // -------------------- query --------------------

    bool HasFlag(const std::string& key) {
        auto it = g_flags.find(key);
        return it != g_flags.end() && it->second;
    }

    bool HasOption(const std::string& key) {
        return g_options.find(key) != g_options.end();
    }

    std::string GetOption(const std::string& key, const std::string& defaultValue) {
        auto it = g_options.find(key);
        if (it != g_options.end()) return it->second;
        return defaultValue;
    }

    const std::vector<std::string>& GetArgs() {
        return g_args;
    }

}