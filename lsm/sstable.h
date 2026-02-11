#pragma once

#include <optional>
#include <algorithm>
#include <memory>
#include <map>
#include <queue>
#include "bloom_filter.h"

using KeyType = std::string;

template <typename ValueType>
struct SSTable {
    using TableType = std::vector<std::pair<KeyType, std::optional<ValueType>>>;

    SSTable(TableType&& table) : table(std::move(table)) {}

    static std::unique_ptr<SSTable> Make(const std::map<KeyType, std::optional<ValueType>>& mp) {
        TableType table(mp.begin(), mp.end());
        return std::make_unique<SSTable>(std::move(table));
    }

    std::pair<bool, std::optional<ValueType>> Find(const KeyType& key) {
        const auto it = std::lower_bound(table.begin(), table.end(), std::make_pair(key, std::nullopt));
        if (it != table.end() && it->first == key) {
            return std::make_pair(true, it->second);
        }
        return std::make_pair(false, std::nullopt);
    }

    BloomFilter MakeBloomFilter() const {
        BloomFilter bloom_filter(table.size());
        for (const auto& [key, value] : table) {
            bloom_filter.Add(key);
        }
        return bloom_filter;
    }

    const TableType table;
};

template<typename ValueType>
std::unique_ptr<SSTable<ValueType>> Merge(std::vector<std::unique_ptr<SSTable<ValueType>>>&& sstables) {
    using PQElement = std::pair<KeyType, size_t>;
    auto cmp = [](const PQElement& a, const PQElement& b) { // pq finds max
        if (a.first != b.first) {
            return a.first > b.first;
        }
        return a.second < b.second;
    };
    std::priority_queue<PQElement, std::vector<PQElement>, decltype(cmp)> pq(cmp);
    typename SSTable<ValueType>::TableType table;
    std::vector<size_t> indices(sstables.size(), 0);
    for (size_t i = 0; i < sstables.size(); ++i) {
        pq.emplace(sstables[i]->table[0].first, i);
    }
    while (!pq.empty()) {
        auto [key, idx] = pq.top();
        pq.pop();
        if (table.empty() || table.back().first != key) {
            table.emplace_back(key, sstables[idx]->table[indices[idx]].second);
        }
        if (++indices[idx] < sstables[idx]->table.size()) {
            pq.emplace(sstables[idx]->table[indices[idx]].first, idx);
        }
    }
    return std::make_unique<SSTable<ValueType>>(std::move(table));
}