
#ifndef TFC_COLORS_H
#define TFC_COLORS_H

#include <string>

namespace Colors {

    const std::string RESET        = "\u001b[0m";
    const std::string CLEAR_SCREEN = "\033[2J\033[1;1H";

    const std::string BOLD   = "\u001b[1m";

    const std::string RED    = "\u001b[31m";
    const std::string YELLOW = "\u001b[33m";
    const std::string GREEN  = "\u001b[32m";
    const std::string GREY   = "\u001b[37m";

    const std::string CHECKMARK = "\u2713";
    const std::string CROSSMARK = "\u274C";
}

#endif //TFC_COLORS_H
