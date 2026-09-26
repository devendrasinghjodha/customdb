#include "api/database.h"
#include "query/query.h"

#include <iostream>
#include <string>

namespace {
void print_help() {
    std::cout << "Commands: PUT key value | GET key | FIND value | DELETE key | BEGIN | COMMIT | ROLLBACK | COMPACT | EXIT\n";
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: customdb <database-file>\n";
        return 2;
    }

    try {
        customdb::Database database(argv[1]);
        customdb::QueryExecutor executor(database);
        std::cout << "CustomDB ready. Type HELP for commands.\n";

        std::string line;
        while (std::cout << "> " && std::getline(std::cin, line)) {
            if (line == "HELP" || line == "help") {
                print_help();
            } else if (line == "EXIT" || line == "exit" || line == "quit") {
                break;
            } else if (!line.empty()) {
                try {
                    std::cout << executor.execute(line) << '\n';
                } catch (const std::exception& error) {
                    std::cout << "ERROR: " << error.what() << '\n';
                }
            }
        }
        database.close();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
