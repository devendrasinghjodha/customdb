#include "security/auth.h"

#include <functional>

namespace customdb {

std::string AuthManager::digest(const std::string& password) {
    return std::to_string(std::hash<std::string>{}(password));
}

bool AuthManager::create_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.emplace(username, digest(password)).second;
}

bool AuthManager::authenticate(const std::string& username, const std::string& password) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = users_.find(username);
    return found != users_.end() && found->second == digest(password);
}

}  // namespace customdb
