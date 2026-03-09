#include <gtest/gtest.h>
#include "wildcard.h"
#include <set>

class WildCardSearchTest : public ::testing::Test {
protected:
    WildCardSearch search_{100, 2};
};

TEST_F(WildCardSearchTest, AddDocumentAndExactSearch) {
    uint32_t doc0 = search_.add_document("hello world test");
    uint32_t doc1 = search_.add_document("hello there");
    uint32_t doc2 = search_.add_document("world peace");

    auto results = search_.search("hello");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], std::make_pair(0u, 0u));  // doc0, pos0
    EXPECT_EQ(results[1], std::make_pair(1u, 0u));  // doc1, pos0
}

TEST_F(WildCardSearchTest, PrefixSearch) {
    uint32_t doc0 = search_.add_document("apple apricot banana");
    uint32_t doc1 = search_.add_document("application appstore");
    uint32_t doc2 = search_.add_document("batman battle");

    auto results = search_.search("app*");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {0, 0},  // apple
        {1, 0},  // application
        {1, 1}   // appstore
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, SuffixSearch) {
    uint32_t doc0 = search_.add_document("running walking jumping");
    uint32_t doc1 = search_.add_document("sing dancing");

    auto results = search_.search("*ing");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {0, 0},  // running
        {0, 1},  // walking
        {0, 2},  // jumping
        {1, 0},  // sing
        {1, 1}   // dancing
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, WildcardMiddle) {
    uint32_t doc0 = search_.add_document("abc def ghi");
    uint32_t doc1 = search_.add_document("axbycz dxeefy gzhyi");
    uint32_t doc2 = search_.add_document("apple banana cherry");

    auto results = search_.search("a*c");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {0, 0},  // abc
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, NoMatches) {
    search_.add_document("foo bar baz");

    auto results = search_.search("xyz*");
    EXPECT_TRUE(results.empty());
}

TEST_F(WildCardSearchTest, MultipleWildcards) {
    search_.add_document("international interpolation");
    search_.add_document("internet interest");

    auto results = search_.search("int*na*al");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {0, 0},
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, CaseInsensitivity) {
    search_.add_document("Hello World");
    auto results = search_.search("hel*");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {0, 0},
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, PositionsCorrect) {
    uint32_t doc = search_.add_document("one two three two four");
    // positions: 0:"one",1:"two",2:"three",3:"two",4:"four"
    auto results = search_.search("t*");
    std::set<std::pair<uint32_t, uint32_t>> expected = {
        {doc, 1},  // two
        {doc, 2},  // three
        {doc, 3}   // two
    };
    std::set<std::pair<uint32_t, uint32_t>> actual(results.begin(), results.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(WildCardSearchTest, EmptyPattern) {
    auto results = search_.search("");
    EXPECT_TRUE(results.empty());
}

TEST_F(WildCardSearchTest, OnlyWildcard) {
    search_.add_document("a b c");
    auto results = search_.search("*");
    EXPECT_TRUE(results.empty());
}