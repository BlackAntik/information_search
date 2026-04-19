#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include "../common/stemming/english_stem.h"

using SparseVector = std::vector<std::pair<uint32_t, double>>;

struct VsmResult {
    uint32_t doc_id;
    double score;
};

class VectorSpaceModel {
public:
    explicit VectorSpaceModel(const std::vector<std::string>& documents);

    std::vector<VsmResult> search(const std::string& query);

    std::vector<std::string> tokenize(const std::string& text);

    double compute_tf(const std::string& term, uint32_t doc_id) const;
    double compute_idf(const std::string& term) const;
    double compute_tf_idf(const std::string& term, uint32_t doc_id) const;

    // метод только для тестов
    const std::vector<std::string>& vocabulary() const;
    const SparseVector& document_vector(uint32_t doc_id) const;
    SparseVector query_vector(const std::string& query);

    static double dot(const SparseVector& a, const SparseVector& b);
    static double norm(const SparseVector& v);

private:
    uint32_t num_docs_ = 0;
    std::unordered_set<std::string> stop_words_;
    stemming::english_stem<> english_stemmer_;

    std::vector<std::unordered_map<std::string, uint32_t>> doc_term_counts_;
    std::vector<uint32_t> doc_lengths_;
    std::unordered_map<std::string, std::unordered_set<uint32_t>> term_doc_ids_;

    std::vector<std::string> vocab_;
    std::unordered_map<std::string, uint32_t> vocab_index_;

    std::vector<SparseVector> doc_vectors_;
    std::vector<double> doc_norms_;

    std::string stem(const std::string& word);
    std::string to_lower(const std::string& s);
    void build();
};
