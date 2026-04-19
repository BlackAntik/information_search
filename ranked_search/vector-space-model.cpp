#include "vector-space-model.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

VectorSpaceModel::VectorSpaceModel(const std::vector<std::string>& documents) {
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

    num_docs_ = static_cast<uint32_t>(documents.size());
    doc_term_counts_.resize(num_docs_);
    doc_lengths_.resize(num_docs_);

    for (uint32_t doc_id = 0; doc_id < num_docs_; ++doc_id) {
        auto terms = tokenize(documents[doc_id]);
        doc_lengths_[doc_id] = static_cast<uint32_t>(terms.size());
        for (const auto& term : terms) {
            doc_term_counts_[doc_id][term]++;
            term_doc_ids_[term].insert(doc_id);
        }
    }

    build();
}

std::string VectorSpaceModel::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string VectorSpaceModel::stem(const std::string& word) {
    if (word.empty()) return "";
    std::wstring wword(word.begin(), word.end());
    english_stemmer_(wword);
    return std::string(wword.begin(), wword.end());
}

std::vector<std::string> VectorSpaceModel::tokenize(const std::string& text) {
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

double VectorSpaceModel::compute_tf(const std::string& term, uint32_t doc_id) const {
    auto term_it = doc_term_counts_[doc_id].find(term);
    if (term_it == doc_term_counts_[doc_id].end()) return 0.0;
    return static_cast<double>(term_it->second) / static_cast<double>(doc_lengths_[doc_id]);
}

double VectorSpaceModel::compute_idf(const std::string& term) const {
    if (num_docs_ == 0) return 0.0;
    auto it = term_doc_ids_.find(term);
    if (it == term_doc_ids_.end()) return 0.0;
    uint32_t df = static_cast<uint32_t>(it->second.size());
    return std::log(static_cast<double>(num_docs_) / static_cast<double>(df));
}

double VectorSpaceModel::compute_tf_idf(const std::string& term, uint32_t doc_id) const {
    return compute_tf(term, doc_id) * compute_idf(term);
}

void VectorSpaceModel::build() {
    vocab_.clear();
    vocab_index_.clear();
    doc_vectors_.clear();
    doc_norms_.clear();

    std::set<std::string> sorted_terms;
    for (const auto& pair : term_doc_ids_) {
        sorted_terms.insert(pair.first);
    }
    uint32_t idx = 0;
    for (const auto& term : sorted_terms) {
        vocab_.push_back(term);
        vocab_index_[term] = idx++;
    }

    doc_vectors_.resize(num_docs_);
    doc_norms_.resize(num_docs_, 0.0);

    for (uint32_t doc_id = 0; doc_id < num_docs_; ++doc_id) {
        SparseVector vec;
        double sum_sq = 0.0;
        for (const auto& term_count : doc_term_counts_[doc_id]) {
            uint32_t term_idx = vocab_index_[term_count.first];
            double val = compute_tf_idf(term_count.first, doc_id);
            if (val > 0.0) {
                vec.push_back({term_idx, val});
                sum_sq += val * val;
            }
        }
        std::sort(vec.begin(), vec.end());
        doc_vectors_[doc_id] = std::move(vec);
        doc_norms_[doc_id] = std::sqrt(sum_sq);
    }
}

const std::vector<std::string>& VectorSpaceModel::vocabulary() const {
    return vocab_;
}

const SparseVector& VectorSpaceModel::document_vector(uint32_t doc_id) const {
    return doc_vectors_[doc_id];
}

SparseVector VectorSpaceModel::query_vector(const std::string& query) {
    auto terms = tokenize(query);
    if (terms.empty()) return {};

    std::unordered_map<std::string, uint32_t> term_counts;
    for (const auto& t : terms) {
        term_counts[t]++;
    }

    SparseVector qvec;
    for (const auto& pair : term_counts) {
        auto it = vocab_index_.find(pair.first);
        if (it == vocab_index_.end()) continue;
        double tf = static_cast<double>(pair.second) / static_cast<double>(terms.size());
        double idf = compute_idf(pair.first);
        double val = tf * idf;
        if (val > 0.0) {
            qvec.push_back({it->second, val});
        }
    }
    std::sort(qvec.begin(), qvec.end());
    return qvec;
}

double VectorSpaceModel::dot(const SparseVector& a, const SparseVector& b) {
    double result = 0.0;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i].first == b[j].first) {
            result += a[i].second * b[j].second;
            ++i;
            ++j;
        } else if (a[i].first < b[j].first) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}

double VectorSpaceModel::norm(const SparseVector& v) {
    double sum = 0.0;
    for (const auto& p : v) {
        sum += p.second * p.second;
    }
    return std::sqrt(sum);
}

std::vector<VsmResult> VectorSpaceModel::search(const std::string& query) {
    auto qvec = query_vector(query);

    double q_norm = norm(qvec);
    if (q_norm == 0.0) return {};

    std::vector<VsmResult> results;
    for (uint32_t doc_id = 0; doc_id < num_docs_; ++doc_id) {
        if (doc_norms_[doc_id] == 0.0) continue;
        double cosine = dot(doc_vectors_[doc_id], qvec) / (doc_norms_[doc_id] * q_norm);
        if (cosine > 0.0) {
            results.push_back({doc_id, cosine});
        }
    }

    std::sort(results.begin(), results.end(), [](const VsmResult& a, const VsmResult& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.doc_id < b.doc_id;
    });

    return results;
}
