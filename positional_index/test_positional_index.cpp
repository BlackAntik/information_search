#include <gtest/gtest.h>
#include "positional_index.h"
#include <string>
#include <vector>

using Result = std::vector<std::pair<uint32_t, uint32_t>>;

class PositionalIndexTest : public ::testing::Test {
protected:
    PositionalIndex index{10, 2};
};

TEST_F(PositionalIndexTest, AddDocument) {
    uint32_t doc0 = index.add_document("hello world");
    EXPECT_EQ(doc0, 0u);

    uint32_t doc1 = index.add_document("foo bar");
    EXPECT_EQ(doc1, 1u);
}

TEST_F(PositionalIndexTest, Tokenize) {
    auto tokens = index.tokenize("Hello, World! Foo-Bar.");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "foo");
    EXPECT_EQ(tokens[3], "bar");
}

TEST_F(PositionalIndexTest, SingleWordQuery) {
    index.add_document("cat sat on mat");
    index.add_document("dog sat on rug");
    index.add_document("cat chased dog");

    auto docs = index.phrase_query("cat");
    Result expected = {{0, 0}, {2, 0}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("dog");
    expected = {{1, 0}, {2, 2}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, TwoWordPhrase) {
    index.add_document("cat sat on mat");
    index.add_document("dog sat on rug");
    index.add_document("cat chased dog");

    auto docs = index.phrase_query("cat sat");
    Result expected = {{0, 0}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("sat on");
    expected = {{0, 1}, {1, 1}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, ThreeWordPhrase) {
    index.add_document("cat sat on mat");
    index.add_document("dog sat on rug");

    auto docs = index.phrase_query("sat on mat");
    Result expected = {{0, 1}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("sat on rug");
    expected = {{1, 1}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, PhraseNotFound) {
    index.add_document("cat sat on mat");
    index.add_document("dog sat on rug");

    auto docs = index.phrase_query("cat on");
    EXPECT_TRUE(docs.empty());

    docs = index.phrase_query("mat sat");
    EXPECT_TRUE(docs.empty());
}

TEST_F(PositionalIndexTest, EmptyQuery) {
    index.add_document("hello world");
    auto docs = index.phrase_query("");
    EXPECT_TRUE(docs.empty());
}

TEST_F(PositionalIndexTest, WordNotInIndex) {
    index.add_document("hello world");
    auto docs = index.phrase_query("missing");
    EXPECT_TRUE(docs.empty());
}

TEST_F(PositionalIndexTest, RepeatedWord) {
    index.add_document("buffalo buffalo buffalo");

    auto docs = index.phrase_query("buffalo buffalo");
    Result expected = {{0, 0}, {0, 1}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("buffalo buffalo buffalo");
    expected = {{0, 0}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, MultipleDocumentsPhrase) {
    index.add_document("quick brown fox jumps");
    index.add_document("lazy brown fox sleeps");
    index.add_document("quick brown dog runs");

    auto docs = index.phrase_query("brown fox");
    Result expected = {{0, 1}, {1, 1}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("quick brown");
    expected = {{0, 0}, {2, 0}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, LargeNumberOfDocuments) {
    for (int i = 0; i < 100; ++i) {
        index.add_document("word" + std::to_string(i) + " common phrase here");
    }
    index.add_document("special common phrase here");

    auto docs = index.phrase_query("common phrase");
    EXPECT_EQ(docs.size(), 101u);
    for (const auto& [doc_id, pos] : docs) {
        EXPECT_EQ(pos, 1u);
    }
}

TEST_F(PositionalIndexTest, AdjacentWordsOnly) {
    index.add_document("alpha beta gamma delta");
    index.add_document("alpha gamma beta delta");

    auto docs = index.phrase_query("alpha beta");
    Result expected = {{0, 0}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("alpha gamma");
    expected = {{1, 0}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("beta delta");
    expected = {{1, 2}};
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, CaseInsensitive) {
    index.add_document("Hello World");
    index.add_document("HELLO WORLD");

    auto docs = index.phrase_query("hello world");
    Result expected = {{0, 0}, {1, 0}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("Hello World");
    EXPECT_EQ(docs, expected);
}

TEST_F(PositionalIndexTest, FullPhraseMatch) {
    index.add_document("the quick brown fox jumps over the lazy dog");

    auto docs = index.phrase_query("quick brown fox");
    Result expected = {{0, 1}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("the lazy dog");
    expected = {{0, 6}};
    EXPECT_EQ(docs, expected);

    docs = index.phrase_query("quick fox");
    EXPECT_TRUE(docs.empty());
}
