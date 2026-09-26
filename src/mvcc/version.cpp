#include "mvcc/version.h"

namespace customdb {

void VersionStore::write(const std::string& key, TransactionId transaction_id, std::optional<std::string> value) {
    versions_[key].push_back({transaction_id, next_sequence_++, std::move(value)});
}

std::optional<std::string> VersionStore::read(const std::string& key, TransactionId snapshot) const {
    const auto found = versions_.find(key);
    if (found == versions_.end()) {
        return std::nullopt;
    }
    const Version* visible = nullptr;
    for (const auto& version : found->second) {
        if (version.transaction_id <= snapshot && (!visible || version.sequence > visible->sequence)) {
            visible = &version;
        }
    }
    return visible ? visible->value : std::nullopt;
}

std::vector<Version> VersionStore::history(const std::string& key) const {
    const auto found = versions_.find(key);
    return found == versions_.end() ? std::vector<Version>{} : found->second;
}

void VersionStore::garbage_collect(TransactionId oldest_snapshot) {
    for (auto iterator = versions_.begin(); iterator != versions_.end();) {
        auto& history = iterator->second;
        std::size_t newest_old_version = history.size();
        for (std::size_t index = 0; index < history.size(); ++index) {
            if (history[index].transaction_id < oldest_snapshot) {
                newest_old_version = index;
            }
        }
        if (newest_old_version < history.size()) {
            history.erase(history.begin(), history.begin() + static_cast<std::ptrdiff_t>(newest_old_version));
        }
        if (history.empty()) {
            iterator = versions_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

}  // namespace customdb
