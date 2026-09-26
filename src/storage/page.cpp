#include "storage/page.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace customdb {

namespace {
constexpr std::size_t kChecksumBytes = sizeof(std::uint32_t);

std::uint32_t checksum_for(const std::array<std::uint8_t, kPageSize>& bytes) {
    std::uint32_t hash = 2166136261u;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index >= 9 && index < 9 + kChecksumBytes) {
            continue;
        }
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    return hash;
}
}  // namespace

Page::Page() { bytes_.fill(0); }

PageId Page::id() const noexcept {
    PageId value = 0;
    std::memcpy(&value, bytes_.data() + kIdOffset, sizeof(value));
    return value;
}

void Page::set_id(PageId id_value) noexcept {
    std::memcpy(bytes_.data() + kIdOffset, &id_value, sizeof(id_value));
}

PageType Page::type() const noexcept {
    return static_cast<PageType>(bytes_[kTypeOffset]);
}

void Page::set_type(PageType type_value) noexcept {
    bytes_[kTypeOffset] = static_cast<std::uint8_t>(type_value);
}

std::uint32_t Page::checksum() const noexcept {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes_.data() + kChecksumOffset, sizeof(value));
    return value;
}

void Page::refresh_checksum() noexcept {
    const auto value = checksum_for(bytes_);
    std::memcpy(bytes_.data() + kChecksumOffset, &value, sizeof(value));
}

bool Page::checksum_valid() const noexcept { return checksum() == checksum_for(bytes_); }

std::array<std::uint8_t, kPageSize>& Page::bytes() noexcept { return bytes_; }
const std::array<std::uint8_t, kPageSize>& Page::bytes() const noexcept { return bytes_; }

std::string Page::payload() const {
    const auto* begin = reinterpret_cast<const char*>(bytes_.data() + kPayloadOffset);
    return std::string(begin, begin + (bytes_.size() - kPayloadOffset));
}

void Page::set_payload(const std::string& value) {
    if (value.size() > bytes_.size() - kPayloadOffset) {
        throw std::invalid_argument("page payload exceeds page capacity");
    }
    std::fill(bytes_.begin() + kPayloadOffset, bytes_.end(), 0);
    std::copy(value.begin(), value.end(), bytes_.begin() + kPayloadOffset);
}

}  // namespace customdb
