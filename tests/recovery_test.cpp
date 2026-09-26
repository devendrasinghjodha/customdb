#include "api/database.h"
#include "wal/wal.h"

#include <cassert>
#include <filesystem>

int main() {
    const std::string path = "customdb-recovery.db";
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    {
        customdb::Database database(path);
        database.put("stable", "yes");
    }
    {
        customdb::Wal wal(path + ".wal");
        wal.append({99, customdb::WalType::Begin, {}, {}});
        wal.append({99, customdb::WalType::Put, "unstable", "no"});
        wal.sync();
    }
    {
        customdb::Database database(path);
        assert(database.get("stable").value() == "yes");
        assert(!database.get("unstable"));
    }
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    return 0;
}
