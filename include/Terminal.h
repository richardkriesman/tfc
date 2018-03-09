
#ifndef TFC_COLORS_H
#define TFC_COLORS_H

#include <string>

namespace Terminal {

    class Colors {
    public:
        static constexpr const auto RED    = "\u001b[31m";
        static constexpr const auto YELLOW = "\u001b[33m";
        static constexpr const auto GREEN  = "\u001b[32m";
        static constexpr const auto GREY   = "\u001b[37m";
    };

    class Cursor {
    public:
        static constexpr const auto HOME = "\r";
        static constexpr const auto SAVE = "\033[s";
        static constexpr const auto RESTORE = "\033[u";
        static constexpr const auto ERASE_EOL = "\033[K";

        static std::string forward(int n) {
            return "\033[" + std::to_string(n) + "C";
        }
    };

    class Decorations {
    public:
        static constexpr const auto BOLD   = "\u001b[1m";
        static constexpr const auto RESET  = "\u001b[0m";
    };

    class Screen {
    public:
        static constexpr const auto CLEAR = "\033[2J\033[1;1H";
    };

    class Symbols {
    public:
        static constexpr const auto CHECKMARK = "\u2713";
        static constexpr const auto CROSSMARK = "\u274C";
    };
}

#endif //TFC_COLORS_H
