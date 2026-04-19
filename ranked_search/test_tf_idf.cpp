#include <gtest/gtest.h>
#include "tf_idf.h"
#include <string>
#include <vector>
#include <cmath>

class TfIdfTest : public ::testing::Test {
protected:
    TfIdf index;
};

TEST_F(TfIdfTest, AddDocument) {
    uint32_t doc0 = index.add_document("hello world");
    EXPECT_EQ(doc0, 0u);

    uint32_t doc1 = index.add_document("foo bar");
    EXPECT_EQ(doc1, 1u);
}

TEST_F(TfIdfTest, Tokenize) {
    auto tokens = index.tokenize("The quick brown foxes are jumping!");
    EXPECT_FALSE(tokens.empty());
    for (const auto& t : tokens) {
        EXPECT_NE(t, "the");
        EXPECT_NE(t, "are");
    }
}

TEST_F(TfIdfTest, TokenizeStopWords) {
    auto tokens = index.tokenize("the a an is are was were");
    EXPECT_TRUE(tokens.empty());
}

TEST_F(TfIdfTest, ComputeTfSingleTerm) {
    index.add_document("cat cat dog");
    auto terms = index.tokenize("cat");
    ASSERT_EQ(terms.size(), 1u);
    double tf = index.compute_tf(terms[0], 0);
    EXPECT_NEAR(tf, 2.0 / 3.0, 1e-9);
}

TEST_F(TfIdfTest, ComputeTfMissingTerm) {
    index.add_document("cat dog");
    auto terms = index.tokenize("bird");
    ASSERT_EQ(terms.size(), 1u);
    double tf = index.compute_tf(terms[0], 0);
    EXPECT_NEAR(tf, 0.0, 1e-9);
}

TEST_F(TfIdfTest, ComputeIdfAllDocuments) {
    index.add_document("cat");
    index.add_document("cat");
    index.add_document("cat");
    auto terms = index.tokenize("cat");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, std::log(3.0 / 3.0), 1e-9);
}

TEST_F(TfIdfTest, ComputeIdfRareTerm) {
    index.add_document("cat dog");
    index.add_document("bird fish");
    index.add_document("cat fish");
    auto terms = index.tokenize("dog");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, std::log(3.0 / 1.0), 1e-9);
}

TEST_F(TfIdfTest, ComputeIdfMissingTerm) {
    index.add_document("cat dog");
    auto terms = index.tokenize("elephant");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, 0.0, 1e-9);
}

TEST_F(TfIdfTest, ComputeTfIdf) {
    index.add_document("cat cat dog");
    index.add_document("bird fish");
    index.add_document("cat fish");

    auto cat_terms = index.tokenize("cat");
    ASSERT_EQ(cat_terms.size(), 1u);
    double tf = index.compute_tf(cat_terms[0], 0);
    double idf = index.compute_idf(cat_terms[0]);
    double tf_idf = index.compute_tf_idf(cat_terms[0], 0);
    EXPECT_NEAR(tf_idf, tf * idf, 1e-9);
}

TEST_F(TfIdfTest, SearchEmpty) {
    index.add_document("hello world");
    auto results = index.search("");
    EXPECT_TRUE(results.empty());
}

TEST_F(TfIdfTest, SearchStopWordsOnly) {
    index.add_document("hello world");
    auto results = index.search("the a an");
    EXPECT_TRUE(results.empty());
}

TEST_F(TfIdfTest, SearchSingleTerm) {
    index.add_document("cat dog bird");
    index.add_document("fish bird");
    index.add_document("cat cat bird");

    auto results = index.search("cat");
    ASSERT_GE(results.size(), 2u);
    EXPECT_GT(results[0].score, 0.0);
    EXPECT_GE(results[0].score, results[1].score);
}

TEST_F(TfIdfTest, SearchRanking) {
    index.add_document("apple banana cherry");
    index.add_document("apple apple apple");
    index.add_document("banana cherry date");

    auto results = index.search("apple");
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].doc_id, 1u);
    EXPECT_GT(results[0].score, results[1].score);
}

TEST_F(TfIdfTest, SearchMultipleTerms) {
    index.add_document("machine learning algorithms");
    index.add_document("deep learning neural networks");
    index.add_document("machine translation systems");
    index.add_document("random unrelated document");

    auto results = index.search("machine learning");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, 0u);
}

TEST_F(TfIdfTest, SearchNoMatch) {
    index.add_document("cat dog bird");
    index.add_document("fish whale shark");

    auto results = index.search("elephant");
    EXPECT_TRUE(results.empty());
}

TEST_F(TfIdfTest, RareTermHigherScore) {
    index.add_document("cat dog");
    index.add_document("cat bird");
    index.add_document("cat fish");
    index.add_document("dog fish");

    auto cat_results = index.search("cat");
    auto dog_results = index.search("dog");

    double max_cat_score = 0.0;
    for (const auto& r : cat_results) max_cat_score = std::max(max_cat_score, r.score);
    double max_dog_score = 0.0;
    for (const auto& r : dog_results) max_dog_score = std::max(max_dog_score, r.score);

    EXPECT_GT(max_dog_score, max_cat_score);
}

TEST_F(TfIdfTest, StemmedSearch) {
    index.add_document("running runners run");
    index.add_document("swimming swimmers swim");

    auto results = index.search("runs");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, 0u);
}

TEST_F(TfIdfTest, CaseInsensitive) {
    index.add_document("Hello Cat");
    index.add_document("HELLO DOG");
    index.add_document("goodbye bird");

    auto results = index.search("hello");
    EXPECT_EQ(results.size(), 2u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.doc_id == 0u || r.doc_id == 1u);
    }
}

TEST_F(TfIdfTest, LargeCorpus) {
    for (int i = 0; i < 100; ++i) {
        index.add_document("generic document number " + std::to_string(i));
    }
    index.add_document("unique special keyword");

    auto results = index.search("keyword");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].doc_id, 100u);
}

TEST_F(TfIdfTest, ScoresDescending) {
    index.add_document("alpha beta gamma");
    index.add_document("alpha alpha beta");
    index.add_document("alpha alpha alpha");
    index.add_document("beta gamma delta");

    auto results = index.search("alpha");
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].score, results[i].score);
    }
}
