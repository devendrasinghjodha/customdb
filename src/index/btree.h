#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace customdb {

class BTree {
public:
    BTree();

    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    bool insert(const std::string& key, const std::string& value);
    std::optional<std::string> search(const std::string& key) const;
    bool erase(const std::string& key);
    std::vector<std::pair<std::string, std::string>> entries() const;
    std::size_t size() const noexcept;

private:
    struct Node {
        bool leaf = true;
        std::vector<std::string> keys;
        std::vector<std::string> values;
        std::vector<std::unique_ptr<Node>> children;
    };

    static constexpr std::size_t kMinimumDegree = 8;
    void split_child(Node& parent, std::size_t child_index);
    void insert_nonfull(Node& node, const std::string& key, const std::string& value);
    std::optional<std::string> search(const Node& node, const std::string& key) const;
    void collect(const Node& node, std::vector<std::pair<std::string, std::string>>& output) const;

    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;
};

}  // namespace customdb
