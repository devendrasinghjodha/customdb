#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace customdb {

class SecondaryIndex {
public:
    void add(const std::string& indexed_value, const std::string& primary_key);
    void remove(const std::string& indexed_value, const std::string& primary_key);
    std::vector<std::string> lookup(const std::string& indexed_value) const;
    void clear();

private:
    std::map<std::string, std::set<std::string>> values_;
};

}  // namespace customdb
