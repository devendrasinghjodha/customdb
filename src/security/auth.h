#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace customdb {

class AuthManager {
public:
    bool create_user(const std::string& username, const std::string& password);
    bool authenticate(const std::string& username, const std::string& password) const;

private:
    static std::string digest(const std::string& password);
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> users_;
};

}  // namespace customdb
