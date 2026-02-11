#pragma once

#include <string>
#include <vector>

class BloomFilter {
public:
    BloomFilter(size_t num_elements) : bits_(num_elements * 10, false) {}

    void Add(const std::string& key) {
        for (size_t i = 0; i < num_hashes_; ++i) {
            bits_[Hash(key, i) % bits_.size()] = true;
        }
    }

    bool Contains(const std::string& key) const {
        for (size_t i = 0; i < num_hashes_; ++i) {
            if (!bits_[Hash(key, i) % bits_.size()]) {
                return false;
            }
        }
        return true;
    }
private:
    const size_t num_hashes_ = 5;
    std::vector<bool> bits_;

    size_t Hash(const std::string& key, size_t seed) const {
        return std::hash<std::string>{}(key + std::to_string(seed));
    }
};