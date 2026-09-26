#include "index/btree.h"

#include <algorithm>

namespace customdb {

 BTree::BTree() : root_(std::make_unique<Node>()) {}

bool BTree::insert(const std::string& key, const std::string& value) {
    if (search(key)) {
        auto* node = root_.get();
        while (true) {
            const auto position = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            if (position != node->keys.end() && *position == key) {
                node->values[static_cast<std::size_t>(position - node->keys.begin())] = value;
                return false;
            }
            if (node->leaf) break;
            node = node->children[static_cast<std::size_t>(position - node->keys.begin())].get();
        }
    }
    if (root_->keys.size() == 2 * kMinimumDegree - 1) {
        auto new_root = std::make_unique<Node>();
        new_root->leaf = false;
        new_root->children.push_back(std::move(root_));
        split_child(*new_root, 0);
        root_ = std::move(new_root);
    }
    insert_nonfull(*root_, key, value);
    ++size_;
    return true;
}

std::optional<std::string> BTree::search(const std::string& key) const {
    return search(*root_, key);
}

bool BTree::erase(const std::string& key) {
    if (!search(key)) return false;
    const auto all = entries();
    root_ = std::make_unique<Node>();
    size_ = 0;
    for (const auto& [entry_key, entry_value] : all) {
        if (entry_key != key) insert(entry_key, entry_value);
    }
    return true;
}

std::vector<std::pair<std::string, std::string>> BTree::entries() const {
    std::vector<std::pair<std::string, std::string>> output;
    output.reserve(size_);
    collect(*root_, output);
    return output;
}

std::size_t BTree::size() const noexcept { return size_; }

void BTree::split_child(Node& parent, std::size_t child_index) {
    auto& full = *parent.children[child_index];
    auto right = std::make_unique<Node>();
    right->leaf = full.leaf;
    const auto median = kMinimumDegree - 1;
    const auto median_key = full.keys[median];
    const auto median_value = full.values[median];
    right->keys.assign(full.keys.begin() + static_cast<std::ptrdiff_t>(kMinimumDegree), full.keys.end());
    right->values.assign(full.values.begin() + static_cast<std::ptrdiff_t>(kMinimumDegree), full.values.end());
    if (!full.leaf) {
        for (std::size_t index = kMinimumDegree; index < full.children.size(); ++index) {
            right->children.push_back(std::move(full.children[index]));
        }
        full.children.resize(kMinimumDegree);
    }
    full.keys.resize(median);
    full.values.resize(median);
    parent.keys.insert(parent.keys.begin() + static_cast<std::ptrdiff_t>(child_index), median_key);
    parent.values.insert(parent.values.begin() + static_cast<std::ptrdiff_t>(child_index), median_value);
    parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1), std::move(right));
}

void BTree::insert_nonfull(Node& node, const std::string& key, const std::string& value) {
    auto position = std::lower_bound(node.keys.begin(), node.keys.end(), key);
    auto index = static_cast<std::size_t>(position - node.keys.begin());
    if (node.leaf) {
        node.keys.insert(position, key);
        node.values.insert(node.values.begin() + static_cast<std::ptrdiff_t>(index), value);
        return;
    }
    if (node.children[index]->keys.size() == 2 * kMinimumDegree - 1) {
        split_child(node, index);
        if (key > node.keys[index]) ++index;
    }
    insert_nonfull(*node.children[index], key, value);
}

std::optional<std::string> BTree::search(const Node& node, const std::string& key) const {
    const auto position = std::lower_bound(node.keys.begin(), node.keys.end(), key);
    const auto index = static_cast<std::size_t>(position - node.keys.begin());
    if (position != node.keys.end() && *position == key) return node.values[index];
    if (node.leaf) return std::nullopt;
    return search(*node.children[index], key);
}

void BTree::collect(const Node& node, std::vector<std::pair<std::string, std::string>>& output) const {
    for (std::size_t index = 0; index < node.keys.size(); ++index) {
        if (!node.leaf) collect(*node.children[index], output);
        output.emplace_back(node.keys[index], node.values[index]);
    }
    if (!node.leaf) collect(*node.children.back(), output);
}

}  // namespace customdb
