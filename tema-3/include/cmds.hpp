#pragma once

#include "Client.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace cmds {
    void reg(Client&);
    void login(Client&);
    void logout(Client&);
    void exit(Client&);

    void lib_enter(Client&);
    void lib_get_books(Client&);
    void lib_get_book(Client&);
    void lib_add_book(Client&);
    void lib_del_book(Client&);

    std::unordered_map<std::string, std::function<void(Client&)>> const from = {
        {"register", reg},
        {"login",    login},
        {"logout",   logout},
        {"exit",     exit},

        {"enter_library", lib_enter},
        {"get_books",     lib_get_books},
        {"get_book",      lib_get_book},
        {"add_book",      lib_add_book},
        {"delete_book",   lib_del_book},
    };
}
