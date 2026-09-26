#pragma once

#include "wal/wal.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace customdb {

class LockManager {
public:
    class Guard {
    public:
        Guard() = default;
        Guard(std::shared_ptr<std::mutex> lock, std::unique_lock<std::mutex> holder);
        Guard(Guard&&) noexcept = default;
        Guard& operator=(Guard&&) noexcept = default;
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

    private:
        std::shared_ptr<std::mutex> lock_;
        std::unique_lock<std::mutex> holder_;
    };

    Guard acquire(const std::string& key, TransactionId transaction_id);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<std::mutex>> locks_;
};

}  // namespace customdb
