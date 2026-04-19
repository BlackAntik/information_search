#include "positional_index.h"
#include <algorithm>
#include <cctype>

PositionalIndex::PositionalIndex(size_t memtable_size, size_t growth_factor)
    : lsm_(memtable_size, growth_factor,
           [](const std::optional<PostingList>& first,
              const std::optional<PostingList>& second) -> std::optional<PostingList> {
               if (!first) return second;
               if (!second) return first;
               PostingList merged = *first;
               merged.insert(merged.end(), second->begin(), second->end());
               return merged;
           }) {}

std::string PositionalIndex::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> PositionalIndex::tokenize(const std::string& text) {
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
        tokens.push_back(lower.substr(start, i - start));
    }
    return tokens;
}

uint32_t PositionalIndex::add_document(const std::string& text) {
    uint32_t doc_id = next_doc_id_++;
    auto words = tokenize(text);
    for (uint32_t position = 0; position < words.size(); ++position) {
        PostingList posting;
        posting.emplace_back(doc_id, position);
        lsm_.Put(words[position], posting);
    }
    return doc_id;
}

PostingList PositionalIndex::get_posting_list(const std::string& term) {
    auto opt = lsm_.Get(term);
    if (opt.has_value()) {
        return opt.value();
    }
    return PostingList{};
}

std::vector<std::pair<uint32_t, uint32_t>> PositionalIndex::intersect_phrase(const std::vector<PostingList>& postings) {
    if (postings.empty()) return {};

    std::vector<size_t> idx(postings.size(), 0);
    std::vector<std::pair<uint32_t, uint32_t>> result;

    while (idx[0] < postings[0].size()) {
        uint32_t doc_id = postings[0][idx[0]].first;
        uint32_t start_pos = postings[0][idx[0]].second;

        bool match = true;
        for (size_t k = 1; k < postings.size(); ++k) {
            uint32_t expected_pos = start_pos + static_cast<uint32_t>(k);
            while (idx[k] < postings[k].size() &&
                   (postings[k][idx[k]].first < doc_id ||
                    (postings[k][idx[k]].first == doc_id && postings[k][idx[k]].second < expected_pos))) {
                ++idx[k];
            }
            if (idx[k] >= postings[k].size() ||
                postings[k][idx[k]].first != doc_id ||
                postings[k][idx[k]].second != expected_pos) {
                match = false;
                break;
            }
        }

        if (match) {
            result.emplace_back(doc_id, start_pos);
        }
        ++idx[0];
    }

    return result;
}

std::vector<std::pair<uint32_t, uint32_t>> PositionalIndex::phrase_query(const std::string& phrase) {
    auto words = tokenize(phrase);
    if (words.empty()) return {};

    std::vector<PostingList> postings;
    for (const auto& word : words) {
        postings.push_back(get_posting_list(word));
        if (postings.back().empty()) {
            return {};
        }
    }

    return intersect_phrase(postings);
}
