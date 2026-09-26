#include "api/database.h"

#include <cassert>
#include <filesystem>
#include <thread>
#include <vector>

int main() {
    const std::string path = "customdb-concurrency.db";
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    customdb::Database database(path);

    std::vector<std::thread> workers;
    for (int worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&database, worker] {
            for (int index = 0; index < 25; ++index) {
                database.put("worker-" + std::to_string(worker) + "-" + std::to_string(index), "value");
            }
        });
    }
    for (auto& worker : workers) worker.join();
    for (int worker = 0; worker < 4; ++worker) {
        for (int index = 0; index < 25; ++index) {
            assert(database.get("worker-" + std::to_string(worker) + "-" + std::to_string(index)).has_value());
        }
    }
    database.close();
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    return 0;
}
