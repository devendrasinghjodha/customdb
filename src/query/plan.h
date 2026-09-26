#pragma once

#include <optional>
#include <string>

namespace customdb {

enum class AccessPath { FullScan, SecondaryIndex };

struct QueryPlan {
    AccessPath access_path = AccessPath::FullScan;
    std::string table;
    std::optional<std::string> equality_value;
};

class QueryPlanner {
public:
    QueryPlan plan_select(const std::string& table, std::optional<std::string> equality_value) const;
};

}  // namespace customdb
