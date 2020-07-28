#include "cmds.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>

#include <cstdint>
#include <cstdlib>

namespace {
    void read_number(std::istream& is,
                     std::string const& prompt,
                     std::int32_t& n)
    {
        while (true) {
            if (&is == &std::cin) {
                std::cout << prompt;
            }

            if ((is >> n && n >= 0) || is.eof()) {
                is.ignore();
                break;
            }

            is.clear();
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    
    struct UserData {
        std::string username;
        std::string password;

        friend std::istream& operator>>(std::istream& is, UserData& d) {
            if (&is == &std::cin) std::cout << "username=";
            std::getline(is, d.username);

            if (&is == &std::cin) std::cout << "password=";
            std::getline(is, d.password);

            return is;
        }
    };

    struct BookData {
        std::string title;
        std::string author;
        std::string genre;
        std::string publisher;
        std::int32_t page_count = 0;

        friend std::istream& operator>>(std::istream& is, BookData& d) {
            if (&is == &std::cin) std::cout << "title=";
            std::getline(is, d.title);

            if (&is == &std::cin) std::cout << "author=";
            std::getline(is, d.author);

            if (&is == &std::cin) std::cout << "genre=";
            std::getline(is, d.genre);

            if (&is == &std::cin) std::cout << "publisher=";
            std::getline(is, d.publisher);

            read_number(is, "page_count=", d.page_count);

            return is;
        }
    };

    struct BookQuery {
        std::int32_t id = 0;

        friend std::istream& operator>>(std::istream& is, BookQuery& q) {
            read_number(is, "id=", q.id);

            return is;
        }
    };
}

void cmds::reg(Client& cl) {
    UserData d;
    std::cin >> d;

    auto const& [code, j] = cl.post(
        "/api/v1/tema/auth/register",
        {
            {"username", d.username},
            {"password", d.password},
        }
    );

    if (code == 201) {
        std::cout << "Account created." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::login(Client& cl) {
    UserData d;
    std::cin >> d;

    auto const& [code, j] = cl.post(
        "/api/v1/tema/auth/login",
        {
            {"username", d.username},
            {"password", d.password},
        }
    );

    if (code == 200) {
        std::cout << "Authenticated." << std::endl;
    } else if (code == 204) {
        std::cerr << "Already authenticated." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::logout(Client& cl) {
    auto const& [code, j] = cl.get("/api/v1/tema/auth/logout");

    if (code == 200) {
        cl.clear();

        std::cout << "Deauthenticated." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::exit(Client& cl) {
    (void)cl;
    std::exit(EXIT_SUCCESS);
}

void cmds::lib_enter(Client& cl) {
    auto const& [code, j] = cl.get("/api/v1/tema/library/access");

    if (code == 200) {
        std::cout << "Access granted." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::lib_get_books(Client& cl) {
    auto const& [code, j] = cl.get("/api/v1/tema/library/books");

    if (code == 200 && j.is_array()) {
        if (j.empty()) {
            std::cout << "There are no books to show." << std::endl;
        }

        for (auto const& book : j) {
            std::cout << book["id"].get<std::int32_t>() << ": "
                      << book["title"] << std::endl;
        }
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::lib_get_book(Client& cl) {
    BookQuery d;
    std::cin >> d;

    auto const& [code, j] = cl.get("/api/v1/tema/library/books/"
                                   +std::to_string(d.id));

    if (code == 200) {
        for (auto const& [k, v] : (j.is_array() ? j.at(0) : j).items()) {
            std::cout << k << ": " << v << std::endl;
        }
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::lib_add_book(Client& cl) {
    BookData d;
    std::cin >> d;

    auto const& [code, j] = cl.post(
        "/api/v1/tema/library/books",
        {
            {"title", d.title},
            {"author", d.author},
            {"genre", d.genre},
            {"publisher", d.publisher},
            {"page_count", d.page_count},
        }
    );

    if (code == 200) {
        std::cout << "Book added." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}

void cmds::lib_del_book(Client& cl) {
    BookQuery d;
    std::cin >> d;

    auto const& [code, j] = cl.del("/api/v1/tema/library/books/"
                                   +std::to_string(d.id));

    if (code == 200) {
        std::cout << "Book deleted." << std::endl;
    } else if (j.contains("error")) {
        std::cerr << j["error"].get<std::string>() << std::endl;
    } else {
        std::cerr << "Something went wrong." << std::endl;
    }
}
