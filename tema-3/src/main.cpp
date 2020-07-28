#include "Client.hpp"
#include "cmds.hpp"

#include <cstdlib>

#include <iostream>
#include <sstream>

namespace {
    Client cl;
}

int main() {
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);

        std::string cmd;
        iss >> cmd;

        if (cmd.empty()) {
            std::cerr << "Missing command." << std::endl;
            continue;
        }

        auto it = cmds::from.find(cmd);

        if (it == cmds::from.end()) {
            std::cerr << "Unknown command." << std::endl;
            continue;
        }

        it->second(cl);
    }

    return EXIT_SUCCESS;
}
