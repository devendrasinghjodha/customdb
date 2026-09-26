#include "index/secondary_index.h"

namespace customdb {

void SecondaryIndex::add(const std::string& indexed_value, const std::string& primary_key) {
    values_[indexed_value].insert(primary_key);
}

void SecondaryIndex::remove(const std::string& indexed_value, const std::string& primary_key) {
    const auto found = values_.find(indexed_value);
    if (found == values_.end()) {
        return;
    }
    found->second.erase(primary_key);
    if (found->second.empty()) {
        values_.erase(found);
    }
}

std::vector<std::string> SecondaryIndex::lookup(const std::string& indexed_value) const {
    const auto found = values_.find(indexed_value);
    if (found == values_.end()) {
        return {};
    }
    return {found->second.begin(), found->second.end()};
}

void SecondaryIndex::clear() { values_.clear(); }

}  // namespace customdb
