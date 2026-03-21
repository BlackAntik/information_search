#include <gtest/gtest.h>
#include "date_index.h"
#include <string>
#include <vector>
#include <algorithm>

class DateIndexTest : public ::testing::Test {
protected:
    DateIndex idx{10, 2};
};

// ============================================================================
// date_to_int
// ============================================================================

TEST_F(DateIndexTest, DateToInt_Epoch) {
    EXPECT_EQ(DateIndex::date_to_int("1970-01-01"), 0);
}

TEST_F(DateIndexTest, DateToInt_KnownDate) {
    int32_t val = DateIndex::date_to_int("2024-01-01");
    EXPECT_GT(val, 0);
    EXPECT_EQ(DateIndex::date_to_int("2024-01-02"), val + 1);
}

TEST_F(DateIndexTest, DateToInt_Ordering) {
    EXPECT_LT(DateIndex::date_to_int("2020-01-01"), DateIndex::date_to_int("2024-01-01"));
    EXPECT_LT(DateIndex::date_to_int("2024-01-01"), DateIndex::date_to_int("2024-06-15"));
    EXPECT_LT(DateIndex::date_to_int("2024-06-15"), DateIndex::date_to_int("2024-12-31"));
}

TEST_F(DateIndexTest, DateToInt_InvalidFormat) {
    EXPECT_THROW(DateIndex::date_to_int("2024/01/01"), std::runtime_error);
    EXPECT_THROW(DateIndex::date_to_int("20240101"), std::runtime_error);
    EXPECT_THROW(DateIndex::date_to_int(""), std::runtime_error);
}

// ============================================================================
// add_document
// ============================================================================

TEST_F(DateIndexTest, AddDocument_ReturnsIncrementingIds) {
    uint32_t id0 = idx.add_document("hello world", "2024-01-01");
    uint32_t id1 = idx.add_document("foo bar", "2024-02-01");
    uint32_t id2 = idx.add_document("baz qux", "2024-03-01", "2024-06-01");
    EXPECT_EQ(id0, 0);
    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);
}

// ============================================================================
// search_created_in_range
// ============================================================================

TEST_F(DateIndexTest, CreatedInRange_Basic) {
    idx.add_document("doc A", "2024-01-15");
    idx.add_document("doc B", "2024-03-10");
    idx.add_document("doc C", "2024-06-20");
    idx.add_document("doc D", "2024-12-01");

    auto docs = idx.search_created_in_range("2024-01-01", "2024-03-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));

    docs = idx.search_created_in_range("2024-07-01", "2024-12-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({3}));

    docs = idx.search_created_in_range("2024-01-01", "2024-12-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1, 2, 3}));
}

TEST_F(DateIndexTest, CreatedInRange_ExactDate) {
    idx.add_document("doc A", "2024-05-15");
    idx.add_document("doc B", "2024-05-16");

    auto docs = idx.search_created_in_range("2024-05-15", "2024-05-15");
    EXPECT_EQ(docs, std::vector<uint32_t>({0}));
}

TEST_F(DateIndexTest, CreatedInRange_NoResults) {
    idx.add_document("doc A", "2024-01-15");
    idx.add_document("doc B", "2024-03-10");

    auto docs = idx.search_created_in_range("2024-06-01", "2024-12-31");
    EXPECT_TRUE(docs.empty());
}

// ============================================================================
// search_valid_in_range
// ============================================================================

TEST_F(DateIndexTest, ValidInRange_WithEndDate) {
    idx.add_document("doc A", "2024-01-01", "2024-06-30");
    idx.add_document("doc B", "2024-04-01", "2024-12-31");
    idx.add_document("doc C", "2024-08-01", "2024-09-30");

    auto docs = idx.search_valid_in_range("2024-05-01", "2024-05-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));

    docs = idx.search_valid_in_range("2024-09-01", "2024-09-30");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({1, 2}));
}

TEST_F(DateIndexTest, ValidInRange_WithoutEndDate) {
    idx.add_document("doc A", "2024-01-01");
    idx.add_document("doc B", "2024-01-01", "2024-06-30");

    auto docs = idx.search_valid_in_range("2024-12-01", "2024-12-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0}));

    docs = idx.search_valid_in_range("2024-03-01", "2024-03-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));
}

TEST_F(DateIndexTest, ValidInRange_BeforeAllDocs) {
    idx.add_document("doc A", "2024-06-01", "2024-12-31");

    auto docs = idx.search_valid_in_range("2024-01-01", "2024-05-31");
    EXPECT_TRUE(docs.empty());
}

// ============================================================================
// query
// ============================================================================

TEST_F(DateIndexTest, Query_TextOnly) {
    idx.add_document("cats and dogs", "2024-01-01");
    idx.add_document("only cats here", "2024-02-01");
    idx.add_document("only dogs here", "2024-03-01");

    auto docs = idx.query("cat");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));

    docs = idx.query("cat AND dog");
    EXPECT_EQ(docs, std::vector<uint32_t>({0}));
}

TEST_F(DateIndexTest, Query_DateRangeOnly) {
    idx.add_document("doc A", "2024-01-01", "2024-06-30");
    idx.add_document("doc B", "2024-04-01", "2024-12-31");

    auto docs = idx.query("DATE_RANGE(2024-05-01, 2024-05-31)");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));
}

TEST_F(DateIndexTest, Query_CreatedInOnly) {
    idx.add_document("doc A", "2024-01-15");
    idx.add_document("doc B", "2024-06-20");

    auto docs = idx.query("CREATED_IN(2024-01-01, 2024-03-31)");
    EXPECT_EQ(docs, std::vector<uint32_t>({0}));
}

TEST_F(DateIndexTest, Query_TextAndDateRange) {
    idx.add_document("cats playing", "2024-01-01", "2024-06-30");
    idx.add_document("dogs running", "2024-01-01", "2024-12-31");
    idx.add_document("cats sleeping", "2024-07-01", "2024-12-31");

    auto docs = idx.query("cat AND DATE_RANGE(2024-05-01, 2024-05-31)");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0}));

    docs = idx.query("cat AND DATE_RANGE(2024-10-01, 2024-10-31)");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({2}));
}

TEST_F(DateIndexTest, Query_TextOrCreatedIn) {
    idx.add_document("cats playing", "2024-01-15");
    idx.add_document("dogs running", "2024-06-20");
    idx.add_document("birds flying", "2024-01-20");

    auto docs = idx.query("cat OR CREATED_IN(2024-06-01, 2024-06-30)");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1}));
}

TEST_F(DateIndexTest, Query_ComplexFormula) {
    idx.add_document("apple banana", "2024-01-01", "2024-06-30");
    idx.add_document("apple cherry", "2024-03-01", "2024-09-30");
    idx.add_document("banana cherry", "2024-06-01", "2024-12-31");
    idx.add_document("apple banana cherry", "2024-01-01");

    auto docs = idx.query("(apple AND banana) AND DATE_RANGE(2024-07-01, 2024-07-31)");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({3}));
}

TEST_F(DateIndexTest, Query_Empty) {
    auto docs = idx.query("");
    EXPECT_TRUE(docs.empty());
}

// ============================================================================
// bit-slice with many dates
// ============================================================================

TEST_F(DateIndexTest, BitSlice_ManyDates) {
    for (int m = 1; m <= 12; ++m) {
        char buf[11];
        snprintf(buf, sizeof(buf), "2024-%02d-15", m);
        idx.add_document("monthly doc " + std::to_string(m), buf);
    }

    auto docs = idx.search_created_in_range("2024-01-01", "2024-03-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1, 2}));

    docs = idx.search_created_in_range("2024-10-01", "2024-12-31");
    std::sort(docs.begin(), docs.end());
    EXPECT_EQ(docs, std::vector<uint32_t>({9, 10, 11}));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
