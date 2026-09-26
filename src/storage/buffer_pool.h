#pragma once

#include "storage/page.h"
#include "storage/pager.h"

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace customdb {

class BufferPool {
public:
    BufferPool(Pager& pager, std::size_t capacity);

    Page& fetch_page(PageId id);
    void unpin_page(PageId id, bool dirty);
    void flush_page(PageId id);
    void flush_all();

private:
    struct Frame {
        Page page;
        std::size_t pin_count = 0;
        bool dirty = false;
        std::list<PageId>::iterator lru_position;
    };

    void evict_one();

    Pager& pager_;
    std::size_t capacity_;
    std::mutex mutex_;
    std::unordered_map<PageId, std::unique_ptr<Frame>> frames_;
    std::list<PageId> lru_;
};

}  // namespace customdb
