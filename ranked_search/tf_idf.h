#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include "../common/stemming/english_stem.h"

struct TfIdfResult {
    uint32_t doc_id;
    double score;
};

class TfIdf {
public:
    TfIdf();

    uint32_t add_document(const std::string& text);

    std::vector<TfIdfResult> search(const std::string& query);

    std::vector<std::string> tokenize(const std::string& text);

    double compute_tf(const std::string& term, uint32_t doc_id);
    double compute_idf(const std::string& term);
    double compute_tf_idf(const std::string& term, uint32_t doc_id);

private:
    uint32_t next_doc_id_ = 0;
    std::unordered_set<std::string> stop_words_;
    stemming::english_stem<> english_stemmer_;

    std::vector<std::unordered_map<std::string, uint32_t>> doc_term_counts_;
    std::vector<uint32_t> doc_lengths_;
    std::unordered_map<std::string, std::unordered_set<uint32_t>> term_doc_ids_;

    std::string stem(const std::string& word);
    std::string to_lower(const std::string& s);
};
