#pragma once

#include "storage/page.h"

#include <fstream>
#include <mutex>
#include <string>

namespace customdb {

class Pager {
public:
    explicit Pager(const std::string& path);
    ~Pager();

    Pager(const Pager&) = delete;
    Pager& operator=(const Pager&) = delete;

    Page read_page(PageId id);
    void write_page(const Page& page);
    PageId allocate_page();
    void flush();
    void close();
    std::size_t page_count() const;

private:
    std::string path_;
    mutable std::mutex mutex_;
    std::fstream file_;
};

}  // namespace customdb
