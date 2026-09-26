#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace customdb {

using PageId = std::uint64_t;
constexpr std::size_t kPageSize = 4096;

enum class PageType : std::uint8_t {
    Free = 0,
    Metadata = 1,
    Leaf = 2,
    Internal = 3,
};

class Page {
public:
    Page();

    PageId id() const noexcept;
    void set_id(PageId id) noexcept;
    PageType type() const noexcept;
    void set_type(PageType type) noexcept;
    std::uint32_t checksum() const noexcept;
    void refresh_checksum() noexcept;
    bool checksum_valid() const noexcept;

    std::array<std::uint8_t, kPageSize>& bytes() noexcept;
    const std::array<std::uint8_t, kPageSize>& bytes() const noexcept;
    std::string payload() const;
    void set_payload(const std::string& value);

private:
    static constexpr std::size_t kIdOffset = 0;
    static constexpr std::size_t kTypeOffset = sizeof(PageId);
    static constexpr std::size_t kChecksumOffset = kTypeOffset + sizeof(PageType);
    static constexpr std::size_t kPayloadOffset = 32;

    std::array<std::uint8_t, kPageSize> bytes_{};
};

}  // namespace customdb
