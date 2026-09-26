#pragma once

#include "wal/wal.h"
#include "transaction/lock_manager.h"

#include <string>
#include <optional>
#include <unordered_set>
#include <vector>

namespace customdb {

class Database;

enum class TransactionState { Active, Committed, RolledBack };

class Transaction {
public:
    Transaction(Database& database, TransactionId id);

    TransactionId id() const noexcept;
    TransactionState state() const noexcept;
    std::optional<std::string> get(const std::string& key) const;
    void put(const std::string& key, const std::string& value);
    void remove(const std::string& key);
    void commit();
    void rollback();

private:
    Database& database_;
    TransactionId id_;
    TransactionState state_ = TransactionState::Active;
    std::vector<WalRecord> pending_;
    std::vector<LockManager::Guard> locks_;
    std::unordered_set<std::string> locked_keys_;
};

}  // namespace customdb
