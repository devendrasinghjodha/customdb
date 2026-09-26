#pragma once

#include "index/btree.h"
#include "index/secondary_index.h"
#include "api/metrics.h"
#include "mvcc/version.h"
#include "storage/pager.h"
#include "transaction/lock_manager.h"
#include "wal/wal.h"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace customdb {

class Transaction;

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    std::optional<std::string> get(const std::string& key, TransactionId snapshot) const;
    bool remove(const std::string& key);
    std::vector<std::string> find_by_value(const std::string& value) const;
    std::vector<std::pair<std::string, std::string>> scan_prefix(const std::string& prefix) const;
    std::unique_ptr<Transaction> begin();
    void compact();
    DatabaseMetrics metrics() const;
    void backup_to(const std::string& directory) const;
    static void restore_from(const std::string& directory, const std::string& database_path);
    void close();

private:
    friend class Transaction;

    void apply(const WalRecord& record);
    void commit_transaction(TransactionId id, const std::vector<WalRecord>& records);
    void recover();
    void persist_snapshot();

    std::string path_;
    std::string data_path_;
    std::string wal_path_;
    mutable std::mutex mutex_;
    BTree tree_;
    SecondaryIndex secondary_index_;
    VersionStore versions_;
    LockManager lock_manager_;
    Pager pager_;
    Wal wal_;
    TransactionId next_transaction_id_ = 1;
    bool closed_ = false;
};

}  // namespace customdb
