#pragma once

#include "wal/wal.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace customdb {

struct Version {
    TransactionId transaction_id = 0;
    std::uint64_t sequence = 0;
    std::optional<std::string> value;
};

class VersionStore {
public:
    void write(const std::string& key, TransactionId transaction_id, std::optional<std::string> value);
    std::optional<std::string> read(const std::string& key, TransactionId snapshot) const;
    std::vector<Version> history(const std::string& key) const;
    void garbage_collect(TransactionId oldest_snapshot);

private:
    std::map<std::string, std::vector<Version>> versions_;
    std::uint64_t next_sequence_ = 1;
};

}  // namespace customdb
