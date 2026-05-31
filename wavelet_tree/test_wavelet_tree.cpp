#include <gtest/gtest.h>
#include "wavelet_tree.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>

namespace {

size_t brute_rank(const std::vector<int>& data, int symbol, size_t i) {
    size_t count = 0;
    for (size_t j = 0; j < i; ++j) {
        if (data[j] == symbol) {
            ++count;
        }
    }
    return count;
}

size_t brute_select(const std::vector<int>& data, int symbol, size_t k) {
    size_t seen = 0;
    for (size_t j = 0; j < data.size(); ++j) {
        if (data[j] == symbol) {
            ++seen;
            if (seen == k) {
                return j;
            }
        }
    }
    return data.size();
}

}

TEST(WaveletTreeTest, AccessReturnsOriginalSymbols) {
    std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6};
    WaveletTree wt(data, 10);
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(wt.access(i), data[i]);
    }
}

TEST(WaveletTreeTest, RankCountsOccurrences) {
    std::vector<int> data = {1, 2, 1, 3, 1, 2, 1};
    WaveletTree wt(data, 4);
    EXPECT_EQ(wt.rank(1, 7), 4u);
    EXPECT_EQ(wt.rank(2, 7), 2u);
    EXPECT_EQ(wt.rank(3, 7), 1u);
    EXPECT_EQ(wt.rank(1, 0), 0u);
    EXPECT_EQ(wt.rank(1, 3), 2u);
}

TEST(WaveletTreeTest, SelectFindsPositions) {
    std::vector<int> data = {1, 2, 1, 3, 1, 2, 1};
    WaveletTree wt(data, 4);
    EXPECT_EQ(wt.select(1, 1), 0u);
    EXPECT_EQ(wt.select(1, 2), 2u);
    EXPECT_EQ(wt.select(1, 3), 4u);
    EXPECT_EQ(wt.select(1, 4), 6u);
    EXPECT_EQ(wt.select(2, 1), 1u);
    EXPECT_EQ(wt.select(3, 1), 3u);
}

TEST(WaveletTreeTest, RankSelectAreInverse) {
    std::vector<int> data = {5, 3, 5, 1, 5, 3, 2, 5};
    WaveletTree wt(data, 6);
    for (int sym = 1; sym <= 5; ++sym) {
        size_t total = wt.rank(sym, data.size());
        for (size_t k = 1; k <= total; ++k) {
            size_t pos = wt.select(sym, k);
            EXPECT_EQ(wt.rank(sym, pos + 1), k);
        }
    }
}

TEST(WaveletTreeTest, SingleSymbolAlphabet) {
    std::vector<int> data = {0, 0, 0, 0};
    WaveletTree wt(data, 1);
    EXPECT_EQ(wt.rank(0, 4), 4u);
    EXPECT_EQ(wt.access(2), 0);
    EXPECT_EQ(wt.select(0, 3), 2u);
}

TEST(WaveletTreeTest, SizeReturnsElementCount) {
    std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6};
    WaveletTree wt(data, 10);
    EXPECT_EQ(wt.size(), 8u);
}

TEST(WaveletTreeTest, SingleElement) {
    std::vector<int> data = {7};
    WaveletTree wt(data, 8);
    EXPECT_EQ(wt.size(), 1u);
    EXPECT_EQ(wt.access(0), 7);
    EXPECT_EQ(wt.rank(7, 1), 1u);
    EXPECT_EQ(wt.rank(7, 0), 0u);
    EXPECT_EQ(wt.select(7, 1), 0u);
}

TEST(WaveletTreeTest, RankOfAbsentSymbol) {
    std::vector<int> data = {1, 3, 5, 7};
    WaveletTree wt(data, 8);
    EXPECT_EQ(wt.rank(0, 4), 0u);
    EXPECT_EQ(wt.rank(2, 4), 0u);
    EXPECT_EQ(wt.rank(4, 4), 0u);
    EXPECT_EQ(wt.rank(6, 4), 0u);
}

TEST(WaveletTreeTest, RankAtEveryPrefix) {
    std::vector<int> data = {2, 0, 2, 1, 0, 2, 1, 1, 0, 2};
    WaveletTree wt(data, 3);
    for (int sym = 0; sym < 3; ++sym) {
        for (size_t i = 0; i <= data.size(); ++i) {
            EXPECT_EQ(wt.rank(sym, i), brute_rank(data, sym, i))
                << "sym=" << sym << " i=" << i;
        }
    }
}

TEST(WaveletTreeTest, SelectForEveryOccurrence) {
    std::vector<int> data = {2, 0, 2, 1, 0, 2, 1, 1, 0, 2};
    WaveletTree wt(data, 3);
    for (int sym = 0; sym < 3; ++sym) {
        size_t total = wt.rank(sym, data.size());
        for (size_t k = 1; k <= total; ++k) {
            EXPECT_EQ(wt.select(sym, k), brute_select(data, sym, k))
                << "sym=" << sym << " k=" << k;
        }
    }
}

TEST(WaveletTreeTest, BoundaryValuesAtAlphabetEdges) {
    std::vector<int> data = {0, 15, 0, 15, 8, 7, 0, 15};
    WaveletTree wt(data, 16);
    EXPECT_EQ(wt.rank(0, 8), 3u);
    EXPECT_EQ(wt.rank(15, 8), 3u);
    EXPECT_EQ(wt.select(0, 1), 0u);
    EXPECT_EQ(wt.select(15, 3), 7u);
    EXPECT_EQ(wt.access(4), 8);
    EXPECT_EQ(wt.access(5), 7);
}

TEST(WaveletTreeTest, AllSameSymbolInLargerAlphabet) {
    std::vector<int> data = {4, 4, 4, 4, 4};
    WaveletTree wt(data, 8);
    EXPECT_EQ(wt.rank(4, 5), 5u);
    EXPECT_EQ(wt.rank(3, 5), 0u);
    EXPECT_EQ(wt.rank(5, 5), 0u);
    for (size_t k = 1; k <= 5; ++k) {
        EXPECT_EQ(wt.select(4, k), k - 1);
    }
}

TEST(WaveletTreeTest, RandomizedConsistency) {
    std::mt19937 rng(12345);
    for (int trial = 0; trial < 20; ++trial) {
        int sigma = 1 + static_cast<int>(rng() % 16);
        size_t n = 1 + rng() % 200;
        std::vector<int> data(n);
        std::uniform_int_distribution<int> dist(0, sigma - 1);
        for (size_t i = 0; i < n; ++i) {
            data[i] = dist(rng);
        }
        WaveletTree wt(data, sigma);
        EXPECT_EQ(wt.size(), n);
        for (size_t i = 0; i < n; ++i) {
            EXPECT_EQ(wt.access(i), data[i]);
        }
        for (int sym = 0; sym < sigma; ++sym) {
            for (size_t i = 0; i <= n; ++i) {
                EXPECT_EQ(wt.rank(sym, i), brute_rank(data, sym, i));
            }
            size_t total = wt.rank(sym, n);
            for (size_t k = 1; k <= total; ++k) {
                EXPECT_EQ(wt.select(sym, k), brute_select(data, sym, k));
            }
        }
    }
}
