#include "query/plan.h"

#include <utility>

namespace customdb {

QueryPlan QueryPlanner::plan_select(const std::string& table, std::optional<std::string> equality_value) const {
    return {equality_value ? AccessPath::SecondaryIndex : AccessPath::FullScan, table, std::move(equality_value)};
}

}  // namespace customdb
