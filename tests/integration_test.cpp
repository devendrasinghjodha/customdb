#include "api/database.h"
#include "query/query.h"
#include "security/auth.h"

#include <cassert>
#include <filesystem>

int main() {
    const std::string path = "customdb-integration.db";
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");

    customdb::Database database(path);
    customdb::QueryExecutor query(database);
    assert(query.execute("CREATE TABLE users ( id INT , name TEXT )") == "TABLE_CREATED");
    assert(query.execute("INSERT INTO users VALUES ( 1 , Alice )").find("INSERTED") == 0);
    assert(query.execute("SELECT * FROM users").find("Alice") != std::string::npos);
    assert(query.execute("UPDATE users SET row = Bob WHERE id = 0") == "UPDATED");
    assert(query.execute("DELETE FROM users WHERE id = 0") == "DELETED");
    assert(query.execute("SELECT * FROM users").empty());
    const auto metrics = database.metrics();
    assert(metrics.key_count > 0);
    const std::string backup = "customdb-backup";
    std::filesystem::remove_all(backup);
    database.backup_to(backup);
    database.close();

    const std::string restored = "customdb-restored.db";
    std::filesystem::remove(restored);
    std::filesystem::remove(restored + ".pages");
    std::filesystem::remove(restored + ".wal");
    customdb::Database::restore_from(backup, restored);
    {
        customdb::Database restored_database(restored);
        assert(restored_database.get("__schema__:users").has_value());
    }
    customdb::AuthManager auth;
    assert(auth.create_user("devendra", "password"));
    assert(auth.authenticate("devendra", "password"));
    assert(!auth.authenticate("devendra", "wrong"));

    std::filesystem::remove(path);
    std::filesystem::remove(path + ".pages");
    std::filesystem::remove(path + ".wal");
    std::filesystem::remove(restored);
    std::filesystem::remove(restored + ".pages");
    std::filesystem::remove(restored + ".wal");
    std::filesystem::remove_all(backup);
    return 0;
}
