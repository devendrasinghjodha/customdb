#include "storage/pager.h"

#include <filesystem>
#include <stdexcept>
#include <vector>

namespace customdb {

Pager::Pager(const std::string& path) : path_(path) {
    file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_) {
        file_.clear();
        file_.open(path_, std::ios::out | std::ios::binary);
        file_.close();
        file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file_) {
        throw std::runtime_error("unable to open pager file: " + path_);
    }
}

Pager::~Pager() { flush(); }

Page Pager::read_page(PageId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    Page page;
    page.set_id(id);
    file_.seekg(static_cast<std::streamoff>(id * kPageSize));
    file_.read(reinterpret_cast<char*>(page.bytes().data()), static_cast<std::streamsize>(kPageSize));
    if (file_.gcount() == 0) {
        file_.clear();
        return page;
    }
    if (file_.gcount() != static_cast<std::streamsize>(kPageSize)) {
        file_.clear();
        throw std::runtime_error("truncated page in pager file");
    }
    if (!page.checksum_valid()) {
        throw std::runtime_error("page checksum mismatch");
    }
    return page;
}

void Pager::write_page(const Page& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    Page page = input;
    page.refresh_checksum();
    file_.seekp(static_cast<std::streamoff>(page.id() * kPageSize));
    file_.write(reinterpret_cast<const char*>(page.bytes().data()), static_cast<std::streamsize>(kPageSize));
    if (!file_) {
        throw std::runtime_error("unable to write page");
    }
    file_.flush();
}

PageId Pager::allocate_page() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto id = static_cast<PageId>(page_count());
    Page page;
    page.set_id(id);
    page.set_type(PageType::Free);
    page.refresh_checksum();
    file_.seekp(static_cast<std::streamoff>(id * kPageSize));
    file_.write(reinterpret_cast<const char*>(page.bytes().data()), static_cast<std::streamsize>(kPageSize));
    file_.flush();
    return id;
}

void Pager::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

void Pager::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::size_t Pager::page_count() const {
    std::error_code error;
    const auto size = std::filesystem::file_size(path_, error);
    if (error) {
        return 0;
    }
    return static_cast<std::size_t>(size / kPageSize);
}

}  // namespace customdb
