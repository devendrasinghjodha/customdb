#include "api/database.h"
#include "index/btree.h"
#include "storage/page.h"

#include <cassert>
#include <filesystem>

int main() {
    customdb::BTree tree;
    for (int index = 0; index < 100; ++index) {
        assert(tree.insert("key-" + std::to_string(index), "value"));
    }
    assert(tree.search("key-42").value() == "value");
    assert(tree.erase("key-42"));
    assert(!tree.search("key-42"));

    customdb::Page page;
    page.set_id(7);
    page.set_type(customdb::PageType::Leaf);
    page.set_payload("payload");
    page.refresh_checksum();
    assert(page.checksum_valid());

    const std::string path = "customdb-smoke.db";
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    {
        customdb::Database database(path);
        database.put("name", "customdb");
        assert(database.get("name").value() == "customdb");
    }
    {
        customdb::Database database(path);
        assert(database.get("name").value() == "customdb");
    }
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    return 0;
}