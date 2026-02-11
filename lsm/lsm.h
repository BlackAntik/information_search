#pragma once

#include "sstable.h"
#include <ranges>

template<typename ValueType>
class LSM {
public:
    LSM(size_t memtable_size, size_t growth_factor) : memtable_size_(memtable_size), growth_factor_(growth_factor), bloom_filters_(1), disk_(1){}

    void Put(const KeyType& key, const ValueType& value) {
        MyPut(key, std::make_optional(value));
    }

    void Delete(const KeyType& key) {
        MyPut(key, std::nullopt);
    }

    std::optional<ValueType> Get(const KeyType& key) const {
        auto it = memtable_.find(key);
        if (it != memtable_.end()) {
            return it->second;
        }
        for (size_t i = 0; i < disk_.size(); ++i) {
            const auto& level = disk_[i];
            for (size_t wrong_ind = 0; wrong_ind < level.size(); ++wrong_ind) {
                size_t j = level.size() - 1 - wrong_ind;
                if (!bloom_filters_[i][j].Contains(key)) continue;
                auto [found, value] = level[j]->Find(key);
                if (found) {
                    return value;
                }
            }
        }
        return std::nullopt;
    }

private:
    const size_t memtable_size_;
    const size_t growth_factor_;
    std::map<KeyType, std::optional<ValueType>> memtable_;
    std::vector<std::vector<BloomFilter>> bloom_filters_;
    std::vector<std::vector<std::unique_ptr<SSTable<ValueType>>>> disk_;

    void MyPut(const KeyType& key, const std::optional<ValueType>& value) {
        memtable_[key] = value;
        if (memtable_.size() >= memtable_size_) {
            disk_[0].push_back(SSTable<ValueType>::Make(memtable_));
            bloom_filters_[0].push_back(disk_[0].back()->MakeBloomFilter());
            memtable_.clear();
        }
        for (size_t i = 0; disk_[i].size() >= growth_factor_; ++i) {
            if (i + 1 >= disk_.size()) {
                disk_.emplace_back();
                bloom_filters_.emplace_back();
            }
            disk_[i + 1].push_back(Merge(std::move(disk_[i])));
            bloom_filters_[i + 1].push_back(disk_[i + 1].back()->MakeBloomFilter());
            disk_[i].clear();
            bloom_filters_[i].clear();
        }
    }
};