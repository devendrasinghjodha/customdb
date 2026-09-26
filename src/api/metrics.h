#pragma once

#include <cstddef>
#include <string>

namespace customdb {

struct DatabaseMetrics {
    std::size_t key_count = 0;
    std::size_t page_count = 0;
    std::size_t wal_bytes = 0;
};

}  // namespace customdb
