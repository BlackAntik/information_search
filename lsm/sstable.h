#pragma once

#include <optional>
#include <algorithm>
#include <memory>
#include <map>
#include <queue>
#include "bloom_filter.h"

using KeyType = std::string;

template <typename ValueType>
class SSTable {
public:
    constexpr static size_t kBlockSize = 4;

    using KeyValueType = std::pair<KeyType, std::optional<ValueType>>;

    using BlockType = std::vector<KeyValueType>;

    using TableType = std::vector<BlockType>;

    void Add(BlockType&& block) {
        table_.push_back(std::move(block));
    }

    BlockType GetBlock(size_t idx) const {
        return table_[idx];
    }

    size_t GetNumBlocks() const {
        return table_.size();
    }

    static std::unique_ptr<SSTable> Make(const std::map<KeyType, std::optional<ValueType>>& mp) {
        auto res = std::make_unique<SSTable>();
        BlockType block;
        for (auto it = mp.begin(); it != mp.end(); ++it) {
            block.emplace_back(it->first, it->second);
            if (block.size() == kBlockSize) {
                res->Add(std::move(block));
                block.clear();
            }
        }
        if (!block.empty()) {
            res->Add(std::move(block));
        }
        return res;
    }

    std::pair<bool, std::optional<ValueType>> Find(const KeyType& key) const {
        int l = -1, r = GetSize();
        while (l + 1 < r) {
            int mid = (l + r) / 2;
            if (GetKeyValue(mid).first <= key) {
                l = mid;
            } else {
                r = mid;
            }
        }
        if (l >= 0 && GetKeyValue(l).first == key) {
            return {true, GetKeyValue(l).second};
        }
        return {false, std::nullopt};
    }

    BloomFilter MakeBloomFilter() const {
        BloomFilter bloom_filter(GetSize());
        for (const auto& block : table_) {
            for (const auto& [key, value] : block) {
                bloom_filter.Add(key);
            }
        }
        return bloom_filter;
    }

private:
    TableType table_;

    size_t GetSize() const {
        return (GetNumBlocks() - 1) * kBlockSize + table_.back().size();
    }

    const KeyValueType& GetKeyValue(int idx) const {
        return table_[idx / kBlockSize][idx % kBlockSize];
    }
};

template<typename ValueType>
std::unique_ptr<SSTable<ValueType>> Merge(std::vector<std::unique_ptr<SSTable<ValueType>>>&& sstables, const std::function<std::optional<ValueType>(const std::optional<ValueType>&, const std::optional<ValueType>&)>& MergeValues) {
    using PQElement = std::pair<KeyType, size_t>;
    auto cmp = [](const PQElement& a, const PQElement& b) { // pq finds max
        if (a.first != b.first) {
            return a.first > b.first;
        }
        return a.second < b.second;
    };
    std::priority_queue<PQElement, std::vector<PQElement>, decltype(cmp)> pq(cmp);

    auto sstable = std::make_unique<SSTable<ValueType>>();

    typename SSTable<ValueType>::BlockType block;
    std::vector<size_t> block_indices(sstables.size(), 0);
    std::vector<size_t> indices(sstables.size(), 0);
    std::vector<typename SSTable<ValueType>::BlockType> blocks(sstables.size());
    for (size_t i = 0; i < sstables.size(); ++i) {
        blocks[i] = sstables[i]->GetBlock(0);
        pq.emplace(blocks[i][0].first, i);
    }

    auto move_index = [&](size_t idx){
         if (++indices[idx] == blocks[idx].size()) {
            if (++block_indices[idx] < sstables[idx]->GetNumBlocks()) {
                blocks[idx] = sstables[idx]->GetBlock(block_indices[idx]);
                indices[idx] = 0;
            } else {
                blocks[idx].clear();
            }
        }
        if (indices[idx] < blocks[idx].size()) {
            pq.emplace(blocks[idx][indices[idx]].first, idx);
        }
    };

    while (!pq.empty()) {
        auto [key, idx] = pq.top();
        pq.pop();
        std::optional<ValueType> cur_value = blocks[idx][indices[idx]].second;
        move_index(idx);
        
        while (!pq.empty() && pq.top().first == key) {
            size_t other_idx = pq.top().second;
            pq.pop();
            cur_value = MergeValues(blocks[other_idx][indices[other_idx]].second, cur_value);
            move_index(other_idx);
        }
        block.emplace_back(key, cur_value);
        if (block.size() == SSTable<ValueType>::kBlockSize) {
            sstable->Add(std::move(block));
            block.clear();
        }
    }
    if (!block.empty()) {
        sstable->Add(std::move(block));
    }
    return sstable;
}