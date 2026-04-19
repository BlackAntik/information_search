#include "tf_idf.h"
#include <algorithm>
#include <cctype>
#include <cmath>

TfIdf::TfIdf() {
    const std::vector<std::string> stops = {
        "i", "me", "my", "myself", "we", "our", "ours", "ourselves", "you", "your", "yours",
        "yourself", "yourselves", "he", "him", "his", "himself", "she", "her", "hers",
        "herself", "it", "its", "itself", "they", "them", "their", "theirs", "themselves",
        "what", "which", "who", "whom", "this", "that", "these", "those", "am", "is", "are",
        "was", "were", "be", "been", "being", "have", "has", "had", "having", "do", "does",
        "did", "doing", "a", "an", "the", "and", "but", "if", "or", "because", "as", "until",
        "while", "of", "at", "by", "for", "with", "about", "against", "between", "into",
        "through", "during", "before", "after", "above", "below", "to", "from", "up",
        "down", "in", "out", "on", "off", "over", "under", "again", "further", "then",
        "once", "here", "there", "when", "where", "why", "how", "all", "any", "both",
        "each", "few", "more", "most", "other", "some", "such", "no", "nor", "not",
        "only", "own", "same", "so", "than", "too", "very", "s", "t", "can", "will",
        "just", "don", "should", "now"
    };
    stop_words_ = std::unordered_set<std::string>(stops.begin(), stops.end());
}

std::string TfIdf::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string TfIdf::stem(const std::string& word) {
    if (word.empty()) return "";
    std::wstring wword(word.begin(), word.end());
    english_stemmer_(wword);
    return std::string(wword.begin(), wword.end());
}

std::vector<std::string> TfIdf::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string lower = to_lower(text);
    size_t i = 0;
    while (i < lower.size()) {
        if (!std::isalpha(static_cast<unsigned char>(lower[i]))) {
            ++i;
            continue;
        }
        size_t start = i;
        while (i < lower.size() && std::isalpha(static_cast<unsigned char>(lower[i]))) ++i;
        std::string word = lower.substr(start, i - start);
        if (stop_words_.count(word)) continue;
        std::string stemmed = stem(word);
        if (!stemmed.empty()) tokens.push_back(stemmed);
    }
    return tokens;
}

uint32_t TfIdf::add_document(const std::string& text) {
    uint32_t doc_id = next_doc_id_++;
    auto terms = tokenize(text);
    doc_lengths_.push_back(static_cast<uint32_t>(terms.size()));
    doc_term_counts_.emplace_back();
    for (const auto& term : terms) {
        doc_term_counts_[doc_id][term]++;
        term_doc_ids_[term].insert(doc_id);
    }
    return doc_id;
}

double TfIdf::compute_tf(const std::string& term, uint32_t doc_id) {
    auto term_it = doc_term_counts_[doc_id].find(term);
    if (term_it == doc_term_counts_[doc_id].end()) return 0.0;
    return static_cast<double>(term_it->second) / static_cast<double>(doc_lengths_[doc_id]);
}

double TfIdf::compute_idf(const std::string& term) {
    uint32_t n = next_doc_id_;
    if (n == 0) return 0.0;
    auto it = term_doc_ids_.find(term);
    if (it == term_doc_ids_.end()) return 0.0;
    uint32_t df = static_cast<uint32_t>(it->second.size());
    return std::log(static_cast<double>(n) / static_cast<double>(df));
}

double TfIdf::compute_tf_idf(const std::string& term, uint32_t doc_id) {
    return compute_tf(term, doc_id) * compute_idf(term);
}

std::vector<TfIdfResult> TfIdf::search(const std::string& query) {
    auto query_terms = tokenize(query);

    std::unordered_set<uint32_t> candidate_docs;
    for (const auto& term : query_terms) {
        auto it = term_doc_ids_.find(term);
        if (it != term_doc_ids_.end()) {
            candidate_docs.insert(it->second.begin(), it->second.end());
        }
    }

    std::vector<TfIdfResult> results;
    for (uint32_t doc_id : candidate_docs) {
        double score = 0.0;
        for (const auto& term : query_terms) {
            score += compute_tf_idf(term, doc_id);
        }
        if (score > 0.0) {
            results.push_back({doc_id, score});
        }
    }

    std::sort(results.begin(), results.end(), [](const TfIdfResult& a, const TfIdfResult& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.doc_id < b.doc_id;
    });

    return results;
}
