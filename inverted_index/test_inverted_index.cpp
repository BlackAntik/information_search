#include <gtest/gtest.h>
#include "inverted_index.h"
#include <string>
#include <vector>


class InvertedIndexTest : public ::testing::Test {
protected:
    InvertedIndex index{10, 2};
};

TEST_F(InvertedIndexTest, Preprocess) {
    std::string text = "The quick brown foxes jumping over the lazy dogs.";
    auto words = index.preprocess(text);
    std::unordered_set<std::string> expected = {"quick", "brown", "fox", "jump", "lazi", "dog"};
    EXPECT_EQ(words.size(), expected.size());
    for (const auto& w : expected) {
        EXPECT_TRUE(words.count(w));
    }
}

TEST_F(InvertedIndexTest, AddDocument) {
    uint32_t doc1 = index.add_document("Hello world");
    EXPECT_EQ(doc1, 0);

    uint32_t doc2 = index.add_document("Hello again");
    EXPECT_EQ(doc2, 1);
}

TEST_F(InvertedIndexTest, Tokenize) {
    std::string query = "(cat OR dog) AND mouse";
    auto tokens = index.tokenize(query);
    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, InvertedIndex::TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(tokens[1].value, "cat");
    EXPECT_EQ(tokens[2].type, InvertedIndex::TokenType::OR_OP);
    EXPECT_EQ(tokens[3].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(tokens[3].value, "dog");
    EXPECT_EQ(tokens[4].type, InvertedIndex::TokenType::RPAREN);
    EXPECT_EQ(tokens[5].type, InvertedIndex::TokenType::AND_OP);
    EXPECT_EQ(tokens[6].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(tokens[6].value, "mouse");
}

TEST_F(InvertedIndexTest, ToRPN) {
    std::string query = "(cat OR dog) AND NOT mouse";
    auto tokens = index.tokenize(query);
    auto rpn = index.to_rpn(tokens);
    ASSERT_EQ(rpn.size(), 6);
    EXPECT_EQ(rpn[0].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(rpn[0].value, "cat");
    EXPECT_EQ(rpn[1].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(rpn[1].value, "dog");
    EXPECT_EQ(rpn[2].type, InvertedIndex::TokenType::OR_OP);
    EXPECT_EQ(rpn[3].type, InvertedIndex::TokenType::WORD);
    EXPECT_EQ(rpn[3].value, "mouse");
    EXPECT_EQ(rpn[4].type, InvertedIndex::TokenType::NOT_OP);
    EXPECT_EQ(rpn[5].type, InvertedIndex::TokenType::AND_OP);
}

TEST_F(InvertedIndexTest, QuerySimple) {
    index.add_document("document about cats");
    index.add_document("document about dogs");
    index.add_document("document about cats and dogs");

    auto docs = index.query("cat");
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 2}));

    docs = index.query("dog");
    EXPECT_EQ(docs, std::vector<uint32_t>({1, 2}));

    docs = index.query("cat AND dog");
    EXPECT_EQ(docs, std::vector<uint32_t>{2});

    docs = index.query("cat OR dog");
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1, 2}));

    docs = index.query("NOT cat");
    EXPECT_EQ(docs, std::vector<uint32_t>{1});
}

TEST_F(InvertedIndexTest, QueryComplex) {
    index.add_document("apple banana cherry");
    index.add_document("apple banana");
    index.add_document("banana cherry");
    index.add_document("apple cherry");

    auto docs = index.query("(apple OR cherry) AND banana");
    EXPECT_EQ(docs, std::vector<uint32_t>({0, 1, 2}));

    docs = index.query("NOT (apple AND cherry)");
    EXPECT_EQ(docs, std::vector<uint32_t>({1, 2}));
}

TEST_F(InvertedIndexTest, InvalidQuery) {
    EXPECT_THROW(index.query("invalid $ token"), std::runtime_error);
    EXPECT_THROW(index.query("(unmatched"), std::runtime_error);
    EXPECT_THROW(index.query("missing operand AND"), std::runtime_error);
}

TEST_F(InvertedIndexTest, EmptyQuery) {
    auto docs = index.query("");
    EXPECT_TRUE(docs.empty());
}

TEST_F(InvertedIndexTest, StopWordsAndStemming) {
    index.add_document("The running foxes");
    auto docs = index.query("run");
    EXPECT_EQ(docs, std::vector<uint32_t>{0});
    docs = index.query("foxes");
    EXPECT_EQ(docs, std::vector<uint32_t>{0});

    docs = index.query("the");
    EXPECT_TRUE(docs.empty());
}