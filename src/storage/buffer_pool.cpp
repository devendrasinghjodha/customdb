#include "storage/buffer_pool.h"

#include <stdexcept>

namespace customdb {

BufferPool::BufferPool(Pager& pager, std::size_t capacity) : pager_(pager), capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("buffer pool capacity must be positive");
    }
}

Page& BufferPool::fetch_page(PageId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = frames_.find(id);
    if (found != frames_.end()) {
        ++found->second->pin_count;
        lru_.erase(found->second->lru_position);
        lru_.push_back(id);
        found->second->lru_position = std::prev(lru_.end());
        return found->second->page;
    }

    if (frames_.size() >= capacity_) {
        evict_one();
    }
    auto frame = std::make_unique<Frame>();
    frame->page = pager_.read_page(id);
    frame->pin_count = 1;
    lru_.push_back(id);
    frame->lru_position = std::prev(lru_.end());
    auto* result = frame.get();
    frames_.emplace(id, std::move(frame));
    return result->page;
}

void BufferPool::unpin_page(PageId id, bool dirty) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = frames_.find(id);
    if (found == frames_.end()) {
        throw std::out_of_range("cannot unpin page that is not in buffer pool");
    }
    found->second->dirty = found->second->dirty || dirty;
    if (found->second->pin_count > 0) {
        --found->second->pin_count;
    }
}

void BufferPool::flush_page(PageId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = frames_.find(id);
    if (found == frames_.end()) {
        return;
    }
    if (found->second->dirty) {
        pager_.write_page(found->second->page);
        found->second->dirty = false;
    }
}

void BufferPool::flush_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, frame] : frames_) {
        if (frame->dirty) {
            pager_.write_page(frame->page);
            frame->dirty = false;
        }
    }
}

void BufferPool::evict_one() {
    for (auto iterator = lru_.begin(); iterator != lru_.end(); ++iterator) {
        const auto found = frames_.find(*iterator);
        if (found != frames_.end() && found->second->pin_count == 0) {
            if (found->second->dirty) {
                pager_.write_page(found->second->page);
            }
            frames_.erase(found);
            lru_.erase(iterator);
            return;
        }
    }
    throw std::runtime_error("all buffer pool pages are pinned");
}

}  // namespace customdb
