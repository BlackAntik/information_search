#include <gtest/gtest.h>
#include "vector-space-model.h"
#include <string>
#include <vector>
#include <cmath>

TEST(VsmTest, Tokenize) {
    VectorSpaceModel index({});
    auto tokens = index.tokenize("The quick brown foxes are jumping!");
    EXPECT_FALSE(tokens.empty());
    for (const auto& t : tokens) {
        EXPECT_NE(t, "the");
        EXPECT_NE(t, "are");
    }
}

TEST(VsmTest, TokenizeStopWords) {
    VectorSpaceModel index({});
    auto tokens = index.tokenize("the a an is are was were");
    EXPECT_TRUE(tokens.empty());
}

TEST(VsmTest, ComputeTfSingleTerm) {
    VectorSpaceModel index({"cat cat dog"});
    auto terms = index.tokenize("cat");
    ASSERT_EQ(terms.size(), 1u);
    double tf = index.compute_tf(terms[0], 0);
    EXPECT_NEAR(tf, 2.0 / 3.0, 1e-9);
}

TEST(VsmTest, ComputeTfMissingTerm) {
    VectorSpaceModel index({"cat dog"});
    auto terms = index.tokenize("bird");
    ASSERT_EQ(terms.size(), 1u);
    double tf = index.compute_tf(terms[0], 0);
    EXPECT_NEAR(tf, 0.0, 1e-9);
}

TEST(VsmTest, ComputeIdfAllDocuments) {
    VectorSpaceModel index({"cat", "cat", "cat"});
    auto terms = index.tokenize("cat");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, std::log(3.0 / 3.0), 1e-9);
}

TEST(VsmTest, ComputeIdfRareTerm) {
    VectorSpaceModel index({"cat dog", "bird fish", "cat fish"});
    auto terms = index.tokenize("dog");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, std::log(3.0 / 1.0), 1e-9);
}

TEST(VsmTest, ComputeIdfMissingTerm) {
    VectorSpaceModel index({"cat dog"});
    auto terms = index.tokenize("elephant");
    ASSERT_EQ(terms.size(), 1u);
    double idf = index.compute_idf(terms[0]);
    EXPECT_NEAR(idf, 0.0, 1e-9);
}

TEST(VsmTest, ComputeTfIdf) {
    VectorSpaceModel index({"cat cat dog", "bird fish", "cat fish"});
    auto cat_terms = index.tokenize("cat");
    ASSERT_EQ(cat_terms.size(), 1u);
    double tf = index.compute_tf(cat_terms[0], 0);
    double idf = index.compute_idf(cat_terms[0]);
    double tf_idf = index.compute_tf_idf(cat_terms[0], 0);
    EXPECT_NEAR(tf_idf, tf * idf, 1e-9);
}

TEST(VsmTest, DocumentVectorSparse) {
    VectorSpaceModel index({"cat dog", "bird fish"});
    const auto& vec = index.document_vector(0);
    EXPECT_FALSE(vec.empty());
    for (const auto& p : vec) {
        EXPECT_GT(p.second, 0.0);
    }
}

TEST(VsmTest, DocumentVectorSorted) {
    VectorSpaceModel index({"cat dog bird fish whale"});
    const auto& vec = index.document_vector(0);
    for (size_t i = 1; i < vec.size(); ++i) {
        EXPECT_LT(vec[i - 1].first, vec[i].first);
    }
}

TEST(VsmTest, QueryVectorSparse) {
    VectorSpaceModel index({"cat dog", "bird fish"});
    auto qvec = index.query_vector("cat");
    EXPECT_FALSE(qvec.empty());
    for (const auto& p : qvec) {
        EXPECT_GT(p.second, 0.0);
    }
}

TEST(VsmTest, QueryVectorSorted) {
    VectorSpaceModel index({"cat dog bird", "fish whale"});
    auto qvec = index.query_vector("cat dog bird");
    for (size_t i = 1; i < qvec.size(); ++i) {
        EXPECT_LT(qvec[i - 1].first, qvec[i].first);
    }
}

TEST(VsmTest, DotProductSparse) {
    SparseVector a = {{0, 1.0}, {2, 3.0}, {5, 2.0}};
    SparseVector b = {{1, 4.0}, {2, 2.0}, {5, 1.0}};
    double result = VectorSpaceModel::dot(a, b);
    EXPECT_NEAR(result, 3.0 * 2.0 + 2.0 * 1.0, 1e-9);
}

TEST(VsmTest, DotProductNoOverlap) {
    SparseVector a = {{0, 1.0}, {2, 3.0}};
    SparseVector b = {{1, 4.0}, {3, 2.0}};
    double result = VectorSpaceModel::dot(a, b);
    EXPECT_NEAR(result, 0.0, 1e-9);
}

TEST(VsmTest, DotProductEmpty) {
    SparseVector a = {};
    SparseVector b = {{1, 4.0}};
    EXPECT_NEAR(VectorSpaceModel::dot(a, b), 0.0, 1e-9);
    EXPECT_NEAR(VectorSpaceModel::dot(b, a), 0.0, 1e-9);
}

TEST(VsmTest, NormSparse) {
    SparseVector v = {{0, 3.0}, {1, 4.0}};
    EXPECT_NEAR(VectorSpaceModel::norm(v), 5.0, 1e-9);
}

TEST(VsmTest, NormEmpty) {
    SparseVector v = {};
    EXPECT_NEAR(VectorSpaceModel::norm(v), 0.0, 1e-9);
}

TEST(VsmTest, SearchEmpty) {
    VectorSpaceModel index({"hello world"});
    auto results = index.search("");
    EXPECT_TRUE(results.empty());
}

TEST(VsmTest, SearchStopWordsOnly) {
    VectorSpaceModel index({"hello world"});
    auto results = index.search("the a an");
    EXPECT_TRUE(results.empty());
}

TEST(VsmTest, SearchSingleTerm) {
    VectorSpaceModel index({"cat dog bird", "fish bird", "cat cat bird"});
    auto results = index.search("cat");
    ASSERT_GE(results.size(), 2u);
    EXPECT_GT(results[0].score, 0.0);
    EXPECT_GE(results[0].score, results[1].score);
}

TEST(VsmTest, SearchRanking) {
    VectorSpaceModel index({"apple banana cherry", "apple apple apple", "banana cherry date"});
    auto results = index.search("apple");
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].doc_id, 1u);
    EXPECT_GT(results[0].score, results[1].score);
}

TEST(VsmTest, SearchMultipleTerms) {
    VectorSpaceModel index({
        "machine learning algorithms",
        "deep learning neural networks",
        "machine translation systems",
        "random unrelated document"
    });
    auto results = index.search("machine learning");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, 0u);
}

TEST(VsmTest, SearchNoMatch) {
    VectorSpaceModel index({"cat dog bird", "fish whale shark"});
    auto results = index.search("elephant");
    EXPECT_TRUE(results.empty());
}

TEST(VsmTest, CosineScoreBounded) {
    VectorSpaceModel index({"cat dog bird", "cat cat cat", "dog dog dog"});
    auto results = index.search("cat dog");
    for (const auto& r : results) {
        EXPECT_GE(r.score, 0.0);
        EXPECT_LE(r.score, 1.0 + 1e-9);
    }
}

TEST(VsmTest, IdenticalDocumentMaxScore) {
    VectorSpaceModel index({"cat dog bird", "fish whale"});
    auto results = index.search("cat dog bird");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, 0u);
    EXPECT_NEAR(results[0].score, 1.0, 1e-6);
}

TEST(VsmTest, RareTermHigherScore) {
    VectorSpaceModel index({"cat dog", "cat bird", "cat fish", "dog fish"});
    auto cat_results = index.search("cat");
    auto dog_results = index.search("dog");

    double max_cat_score = 0.0;
    for (const auto& r : cat_results) max_cat_score = std::max(max_cat_score, r.score);
    double max_dog_score = 0.0;
    for (const auto& r : dog_results) max_dog_score = std::max(max_dog_score, r.score);

    EXPECT_GT(max_dog_score, max_cat_score);
}

TEST(VsmTest, StemmedSearch) {
    VectorSpaceModel index({"running runners run", "swimming swimmers swim"});
    auto results = index.search("runs");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].doc_id, 0u);
}

TEST(VsmTest, CaseInsensitive) {
    VectorSpaceModel index({"Hello Cat", "HELLO DOG", "goodbye bird"});
    auto results = index.search("hello");
    EXPECT_EQ(results.size(), 2u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.doc_id == 0u || r.doc_id == 1u);
    }
}

TEST(VsmTest, LargeCorpus) {
    std::vector<std::string> docs;
    for (int i = 0; i < 100; ++i) {
        docs.push_back("generic document number " + std::to_string(i));
    }
    docs.push_back("unique special keyword");
    VectorSpaceModel index(docs);

    auto results = index.search("keyword");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].doc_id, 100u);
}

TEST(VsmTest, ScoresDescending) {
    VectorSpaceModel index({"alpha beta gamma", "alpha alpha beta", "alpha alpha alpha", "beta gamma delta"});
    auto results = index.search("alpha");
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].score, results[i].score);
    }
}

TEST(VsmTest, EmptyCorpus) {
    VectorSpaceModel index({});
    auto results = index.search("cat");
    EXPECT_TRUE(results.empty());
}

