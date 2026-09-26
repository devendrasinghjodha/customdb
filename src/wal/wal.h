#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace customdb {

using TransactionId = std::uint64_t;

enum class WalType { Put, Delete, Begin, Commit, Rollback };

struct WalRecord {
    TransactionId transaction_id = 0;
    WalType type = WalType::Put;
    std::string key;
    std::string value;
};

class Wal {
public:
    explicit Wal(const std::string& path);

    void append(const WalRecord& record);
    void sync();
    void truncate();
    std::vector<WalRecord> read_all() const;
    std::uint64_t next_transaction_id() const;

private:
    std::string path_;
};

}  // namespace customdb
