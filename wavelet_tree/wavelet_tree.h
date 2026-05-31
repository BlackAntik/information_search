#pragma once

#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <algorithm>

class WaveletTree {
public:
    WaveletTree() = default;

    WaveletTree(const std::vector<int>& data, int sigma) : data_(data), n_(data.size()), sigma_(sigma) {
        root_ = build(data, 0, sigma_ - 1);
    }

    size_t size() const {
        return n_;
    }

    int access(size_t i) const {
        return data_[i];
    }

    size_t rank(int symbol, size_t i) const {
        int node = root_;
        while (nodes_[node].lo != nodes_[node].hi) {
            const Node& nd = nodes_[node];
            int mid = midpoint(nd.lo, nd.hi);
            size_t ones_before = nd.prefix[i];
            if (symbol <= mid) {
                i -= ones_before;
                node = nd.left;
            } else {
                i = ones_before;
                node = nd.right;
            }
        }
        return i;
    }

    size_t select(int symbol, size_t k) const {
        int node = leaf_of(symbol);
        size_t pos = k - 1;
        while (nodes_[node].parent >= 0) {
            int parent = nodes_[node].parent;
            int bit = (nodes_[parent].right == node) ? 1 : 0;
            pos = select_bit(parent, pos + 1, bit);
            node = parent;
        }
        return pos;
    }

private:
    struct Node {
        int lo = 0;
        int hi = 0;
        int left = -1;
        int right = -1;
        int parent = -1;
        std::vector<size_t> prefix;
    };

    static int midpoint(int lo, int hi) {
        return (lo + hi) / 2;
    }

    int build(const std::vector<int>& data, int lo, int hi) {
        int idx = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{});
        nodes_[idx].lo = lo;
        nodes_[idx].hi = hi;
        if (lo == hi) {
            return idx;
        }
        int mid = midpoint(lo, hi);
        std::vector<size_t> prefix;
        prefix.reserve(data.size() + 1);
        prefix.push_back(0);
        std::vector<int> left_data;
        std::vector<int> right_data;
        for (int v : data) {
            int bit = v > mid ? 1 : 0;
            prefix.push_back(prefix.back() + static_cast<size_t>(bit));
            if (bit) {
                right_data.push_back(v);
            } else {
                left_data.push_back(v);
            }
        }
        nodes_[idx].prefix = std::move(prefix);
        int l = build(left_data, lo, mid);
        int r = build(right_data, mid + 1, hi);
        nodes_[idx].left = l;
        nodes_[idx].right = r;
        nodes_[l].parent = idx;
        nodes_[r].parent = idx;
        return idx;
    }

    int leaf_of(int symbol) const {
        int node = root_;
        while (nodes_[node].lo != nodes_[node].hi) {
            const Node& nd = nodes_[node];
            int mid = midpoint(nd.lo, nd.hi);
            node = (symbol <= mid) ? nd.left : nd.right;
        }
        return node;
    }

    size_t select_bit(int node, size_t t, int bit) const {
        const Node& nd = nodes_[node];
        size_t l = 0;
        size_t r = nd.prefix.size() - 1;
        while (l < r) {
            size_t m = (l + r) / 2;
            size_t cnt;
            if (bit == 1) {
                cnt = nd.prefix[m + 1];
            } else {
                cnt = (m + 1) - nd.prefix[m + 1];
            }
            if (cnt >= t) {
                r = m;
            } else {
                l = m + 1;
            }
        }
        return l;
    }


    std::vector<int> data_;
    size_t n_ = 0;
    int sigma_ = 0;
    int root_ = -1;
    std::vector<Node> nodes_;
};
