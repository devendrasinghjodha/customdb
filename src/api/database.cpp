#include "api/database.h"

#include "transaction/transaction.h"

#include <filesystem>
#include <array>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <unordered_set>

namespace customdb {

Database::Database(const std::string& path)
    : path_(path), data_path_(path + ".pages"), wal_path_(path + ".wal"), pager_(data_path_), wal_(wal_path_) {
    recover();
    next_transaction_id_ = wal_.next_transaction_id();
}

Database::~Database() {
    try {
        close();
    } catch (...) {
    }
}

void Database::put(const std::string& key, const std::string& value) {
    auto transaction = begin();
    transaction->put(key, value);
    transaction->commit();
}

std::optional<std::string> Database::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tree_.search(key);
}

std::optional<std::string> Database::get(const std::string& key, TransactionId snapshot) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return versions_.read(key, snapshot);
}

bool Database::remove(const std::string& key) {
    const bool existed = get(key).has_value();
    auto transaction = begin();
    transaction->remove(key);
    transaction->commit();
    return existed;
}

std::vector<std::string> Database::find_by_value(const std::string& value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return secondary_index_.lookup(value);
}

std::vector<std::pair<std::string, std::string>> Database::scan_prefix(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& entry : tree_.entries()) {
        if (entry.first.rfind(prefix, 0) == 0) {
            result.push_back(entry);
        }
    }
    return result;
}

std::unique_ptr<Transaction> Database::begin() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        throw std::logic_error("database is closed");
    }
    return std::make_unique<Transaction>(*this, next_transaction_id_++);
}

void Database::compact() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        throw std::logic_error("database is closed");
    }
    persist_snapshot();
    wal_.truncate();
}

DatabaseMetrics Database::metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code error;
    return {tree_.size(), pager_.page_count(), std::filesystem::file_size(wal_path_, error)};
}

void Database::backup_to(const std::string& directory) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) throw std::runtime_error("unable to create backup directory");
    const std::array<std::string, 3> sources = {path_, data_path_, wal_path_};
    const std::array<std::string, 3> names = {"snapshot", "pages", "wal"};
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const auto destination = std::filesystem::path(directory) / names[index];
        std::filesystem::copy_file(sources[index], destination, std::filesystem::copy_options::overwrite_existing, error);
        if (error) throw std::runtime_error("unable to create database backup");
    }
}

void Database::restore_from(const std::string& directory, const std::string& database_path) {
    std::error_code error;
    const std::array<std::string, 3> names = {"snapshot", "pages", "wal"};
    const std::array<std::string, 3> suffixes = {"", ".pages", ".wal"};
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto source = std::filesystem::path(directory) / names[index];
        const auto destination = database_path + suffixes[index];
        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, error);
        if (error) throw std::runtime_error("unable to restore database backup");
    }
}

void Database::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!closed_) {
        persist_snapshot();
        pager_.close();
        closed_ = true;
    }
}

void Database::apply(const WalRecord& record) {
    if (record.type == WalType::Put) {
        const auto previous = tree_.search(record.key);
        if (previous) {
            secondary_index_.remove(*previous, record.key);
        }
        tree_.insert(record.key, record.value);
        secondary_index_.add(record.value, record.key);
        versions_.write(record.key, record.transaction_id, record.value);
    } else if (record.type == WalType::Delete) {
        const auto previous = tree_.search(record.key);
        if (previous) {
            secondary_index_.remove(*previous, record.key);
        }
        tree_.erase(record.key);
        versions_.write(record.key, record.transaction_id, std::nullopt);
    }
}

void Database::commit_transaction(TransactionId id, const std::vector<WalRecord>& records) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        throw std::logic_error("database is closed");
    }
    for (const auto& record : records) {
        wal_.append(record);
    }
    wal_.append({id, WalType::Commit, {}, {}});
    wal_.sync();
    for (const auto& record : records) {
        apply(record);
    }
    persist_snapshot();
}

void Database::recover() {
    std::ifstream snapshot(path_, std::ios::binary);
    std::string operation;
    std::string key;
    std::string value;
    while (snapshot >> operation >> std::quoted(key)) {
        if (operation == "PUT") {
            snapshot >> std::quoted(value);
            apply({0, WalType::Put, key, value});
        } else if (operation == "DELETE") {
            tree_.erase(key);
        }
    }
    snapshot.close();

    std::unordered_set<TransactionId> committed;
    const auto records = wal_.read_all();
    for (const auto& record : records) {
        if (record.type == WalType::Commit) {
            committed.insert(record.transaction_id);
        }
    }
    for (const auto& record : records) {
        if (committed.count(record.transaction_id) != 0) {
            apply(record);
        }
    }
    persist_snapshot();
}

void Database::persist_snapshot() {
    const std::string temporary_path = path_ + ".tmp";
    std::ofstream output(temporary_path, std::ios::trunc | std::ios::binary);
    if (!output) {
        throw std::runtime_error("unable to write database snapshot");
    }
    for (const auto& [key, value] : tree_.entries()) {
        output << "PUT " << std::quoted(key) << ' ' << std::quoted(value) << '\n';
    }
    output.flush();
    if (!output) {
        throw std::runtime_error("unable to flush database snapshot");
    }
    output.close();
    std::error_code error;
    std::filesystem::rename(temporary_path, path_, error);
    if (error) {
        std::filesystem::remove(path_, error);
        std::filesystem::rename(temporary_path, path_, error);
    }
    if (error) {
        throw std::runtime_error("unable to replace database snapshot");
    }

    const auto entries = tree_.entries();
    std::string page_payload;
    for (const auto& [key, value] : entries) {
        page_payload += key;
        page_payload.push_back('\t');
        page_payload += value;
        page_payload.push_back('\n');
    }
    const auto pages_needed = std::max<std::size_t>(1, (page_payload.size() + kPageSize - 33) / (kPageSize - 32));
    while (pager_.page_count() < pages_needed) {
        pager_.allocate_page();
    }
    for (std::size_t page_index = 0; page_index < pages_needed; ++page_index) {
        auto page = pager_.read_page(static_cast<PageId>(page_index));
        page.set_type(page_index == 0 ? PageType::Metadata : PageType::Leaf);
        const auto offset = page_index * (kPageSize - 32);
        page.set_payload(page_payload.substr(offset, kPageSize - 32));
        pager_.write_page(page);
    }
}

}  // namespace customdb
