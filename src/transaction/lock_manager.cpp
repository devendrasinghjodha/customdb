#include "transaction/lock_manager.h"

namespace customdb {

LockManager::Guard::Guard(std::shared_ptr<std::mutex> lock, std::unique_lock<std::mutex> holder)
    : lock_(std::move(lock)), holder_(std::move(holder)) {}

LockManager::Guard LockManager::acquire(const std::string& key, TransactionId) {
    std::shared_ptr<std::mutex> lock;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto& entry = locks_[key];
        if (!entry) {
            entry = std::make_shared<std::mutex>();
        }
        lock = entry;
    }
    return Guard(lock, std::unique_lock<std::mutex>(*lock));
}

}  // namespace customdb
