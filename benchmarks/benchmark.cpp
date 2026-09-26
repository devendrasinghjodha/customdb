#include "api/database.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

int main() {
    const std::string path = "customdb-benchmark.db";
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");

    customdb::Database database(path);
    constexpr int operations = 1000;
    const auto started = std::chrono::steady_clock::now();
    for (int index = 0; index < operations; ++index) {
        database.put("key-" + std::to_string(index), "value-" + std::to_string(index));
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    std::cout << "PUT operations: " << operations << "\n"
              << "seconds: " << elapsed << "\n"
              << "ops/sec: " << static_cast<double>(operations) / elapsed << "\n";
    database.close();
    return 0;
}
