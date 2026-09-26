#include "transaction/transaction.h"

#include "api/database.h"

#include <stdexcept>

namespace customdb {

Transaction::Transaction(Database& database, TransactionId id) : database_(database), id_(id) {
    database_.wal_.append({id_, WalType::Begin, {}, {}});
    database_.wal_.sync();
}

TransactionId Transaction::id() const noexcept { return id_; }
TransactionState Transaction::state() const noexcept { return state_; }

std::optional<std::string> Transaction::get(const std::string& key) const {
    if (state_ != TransactionState::Active) {
        throw std::logic_error("transaction is no longer active");
    }
    return database_.get(key, id_);
}

void Transaction::put(const std::string& key, const std::string& value) {
    if (state_ != TransactionState::Active) {
        throw std::logic_error("transaction is no longer active");
    }
    if (locked_keys_.insert(key).second) {
        locks_.push_back(database_.lock_manager_.acquire(key, id_));
    }
    pending_.push_back({id_, WalType::Put, key, value});
}

void Transaction::remove(const std::string& key) {
    if (state_ != TransactionState::Active) {
        throw std::logic_error("transaction is no longer active");
    }
    if (locked_keys_.insert(key).second) {
        locks_.push_back(database_.lock_manager_.acquire(key, id_));
    }
    pending_.push_back({id_, WalType::Delete, key, {}});
}

void Transaction::commit() {
    if (state_ != TransactionState::Active) {
        throw std::logic_error("transaction is no longer active");
    }
    database_.commit_transaction(id_, pending_);
    state_ = TransactionState::Committed;
}

void Transaction::rollback() {
    if (state_ != TransactionState::Active) {
        throw std::logic_error("transaction is no longer active");
    }
    database_.wal_.append({id_, WalType::Rollback, {}, {}});
    database_.wal_.sync();
    state_ = TransactionState::RolledBack;
}

}  // namespace customdb
