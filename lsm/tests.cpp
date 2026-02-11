#include "lsm.h"
#include <gtest/gtest.h>

TEST(LSMTest, BasicTest) {
    LSM<int> lsm(10, 10);
    lsm.Put("a", 1);
    lsm.Put("b", 2);
    EXPECT_EQ(lsm.Get("a"), std::make_optional(1));
    EXPECT_EQ(lsm.Get("b"), std::make_optional(2));

    lsm.Put("c", 3);
    EXPECT_EQ(lsm.Get("a"), std::make_optional(1));
    EXPECT_EQ(lsm.Get("b"), std::make_optional(2));
    EXPECT_EQ(lsm.Get("c"), std::make_optional(3));

    lsm.Delete("b");
    EXPECT_EQ(lsm.Get("a"), std::make_optional(1));
    EXPECT_EQ(lsm.Get("b"), std::nullopt);
    EXPECT_EQ(lsm.Get("c"), std::make_optional(3));
}

TEST(LSMTest, EmptyLSM) {
    LSM<int> lsm(10, 10);
    EXPECT_EQ(lsm.Get("nonexistent"), std::nullopt);
}

TEST(LSMTest, DeleteOnly) {
    LSM<int> lsm(5, 2);
    lsm.Delete("x");
    EXPECT_EQ(lsm.Get("x"), std::nullopt);
    lsm.Put("x", 42);
    EXPECT_EQ(lsm.Get("x"), std::make_optional(42));
}

TEST(LSMTest, FlushMemtable) {
    LSM<int> lsm(3, 10);
    lsm.Put("a", 1);
    lsm.Put("b", 2);
    lsm.Put("c", 3);

    EXPECT_EQ(lsm.Get("a"), 1);
    EXPECT_EQ(lsm.Get("b"), 2);
    EXPECT_EQ(lsm.Get("c"), 3);

    lsm.Put("d", 4);
    EXPECT_EQ(lsm.Get("d"), 4);
}

TEST(LSMTest, CompactionLevel0to1) {
    LSM<int> lsm(2, 2);

    lsm.Put("a", 10);
    lsm.Put("b", 20);
    lsm.Put("c", 30);
    lsm.Put("d", 40);

    EXPECT_EQ(lsm.Get("a"), 10);
    EXPECT_EQ(lsm.Get("b"), 20);
    EXPECT_EQ(lsm.Get("c"), 30);
    EXPECT_EQ(lsm.Get("d"), 40);
}

TEST(LSMTest, LatestValueWinsAfterCompaction) {
    LSM<int> lsm(2, 2);

    lsm.Put("key", 1);
    lsm.Put("key", 2);
    lsm.Put("key", 3);
    lsm.Put("key", 4);

    EXPECT_EQ(lsm.Get("key"), std::make_optional(4));
}

TEST(LSMTest, TombstoneSurvivesCompaction) {
    LSM<int> lsm(2, 2);

    lsm.Put("key", 100);
    lsm.Put("key", 200);
    lsm.Delete("key");
    lsm.Put("other", 999);

    EXPECT_EQ(lsm.Get("key"), std::nullopt);
    EXPECT_EQ(lsm.Get("other"), 999);
}

TEST(LSMTest, MultipleLevelsAndBloom) {
    LSM<int> lsm(2, 3);

    for (int i = 0; i < 6; ++i) {
        lsm.Put("k" + std::to_string(i), i);
    }
    for (int i = 6; i < 12; ++i) {
        lsm.Put("k" + std::to_string(i), i);
    }

    for (int i = 0; i < 12; ++i) {
        EXPECT_EQ(lsm.Get("k" + std::to_string(i)), std::make_optional(i));
    }
    EXPECT_EQ(lsm.Get("missing"), std::nullopt);
}

TEST(LSMTest, DeleteThenPutAgain) {
    LSM<int> lsm(3, 2);

    lsm.Put("a", 1);
    lsm.Delete("a");
    lsm.Put("a", 99);

    EXPECT_EQ(lsm.Get("a"), std::make_optional(99));
}

TEST(LSMTest, LargeMemtableNoFlush) {
    LSM<int> lsm(100, 10);
    for (int i = 0; i < 50; ++i) {
        lsm.Put("k" + std::to_string(i), i);
    }
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(lsm.Get("k" + std::to_string(i)), std::make_optional(i));
    }
}

TEST(LSMTest, Fuzzing) {
    for (size_t memtable_size : {2, 5, 10, 50}) {
        for (size_t growth_factor : {2, 3, 5}) {
            for (size_t num_keys : {1, 10, 100, 10000}) {
                for (size_t num_ops : {10, 100, 1000}) {
                    LSM<int> lsm(memtable_size, growth_factor);
                    std::map<std::string, std::optional<int>> right_ans;
                    for (size_t _ = 0; _ < num_ops; ++_) {
                        int op = rand() % 3;
                        std::string key = "k" + std::to_string(rand() % num_keys);

                        if (op == 0) {
                            int value = rand() % 1000;
                            lsm.Put(key, value);
                            right_ans[key] = value;
                        } else if (op == 1) {
                            lsm.Delete(key);
                            right_ans[key] = std::nullopt;
                        } else {
                            auto expected = right_ans.count(key) ? right_ans[key] : std::nullopt;
                            EXPECT_EQ(lsm.Get(key), expected);
                        }
                    }
                }
            }
        }
    }
}