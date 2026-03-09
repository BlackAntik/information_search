#include "wildcard.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <map>

namespace {

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

}  // namespace

WildCardSearch::WildCardSearch(size_t memtable_size, size_t growth_factor)
    : lsm_(memtable_size, growth_factor,
           [](const std::optional<std::vector<std::pair<uint32_t, uint32_t>>>& a,
              const std::optional<std::vector<std::pair<uint32_t, uint32_t>>>& b)
              -> std::optional<std::vector<std::pair<uint32_t, uint32_t>>> {
               if (!a.has_value()) return b;
               if (!b.has_value()) return a;
               auto res = a.value();
               res.insert(res.end(), b.value().begin(), b.value().end());
               return res;
           }),
      kgram_lsm_(memtable_size, growth_factor,
                 [](const std::optional<roaring::Roaring>& first, const std::optional<roaring::Roaring>& second){
                    if (!first || !second) {
                        throw std::runtime_error("Unexpected input values");
                    }
                    return *first | *second;
                 }) {}

uint32_t WildCardSearch::add_document(const std::string& text) {
    uint32_t doc_id = next_doc_id_++;
    documents_.push_back(text);

    std::string lower_text = to_lower(text);
    std::istringstream iss(lower_text);
    std::string word;
    std::vector<std::pair<std::string, uint32_t>> words_with_pos;
    uint32_t pos = 0;
    while (iss >> word) {
        words_with_pos.emplace_back(std::move(word), pos++);
    }

    for (const auto& [term, position] : words_with_pos) {
        uint32_t term_id;
        bool is_new_term = false;

        auto it = term_to_id_.find(term);
        if (it == term_to_id_.end()) {
            term_id = next_term_id_++;
            term_to_id_[term] = term_id;
            id_to_term_.push_back(term);
            is_new_term = true;
        } else {
            term_id = it->second;
        }

        std::vector<std::pair<uint32_t, uint32_t>> new_posting{{doc_id, position}};
        lsm_.Put(term, std::move(new_posting));

        if (is_new_term) {
            std::string padded = "$" + term + "$";
            for (size_t i = 0; i + k_ <= padded.size(); ++i) {
                std::string gram = padded.substr(i, k_);

                roaring::Roaring bm;
                bm.add(term_id);
                kgram_lsm_.Put(gram, bm);
            }
        }
    }

    return doc_id;
}

std::vector<std::pair<uint32_t, uint32_t>> WildCardSearch::search(const std::string& pattern) {
    std::string pat = to_lower(pattern);
    if (pat.empty()) {
        return {};
    }

    std::vector<std::string> fixed_parts;
    size_t pos = 0;
    while (pos < pat.size()) {
        size_t star = pat.find('*', pos);
        if (star == std::string::npos) {
            fixed_parts.push_back(pat.substr(pos));
            break;
        }
        fixed_parts.push_back(pat.substr(pos, star - pos));
        pos = star + 1;
    }

    bool has_content = false;
    for (const auto& part : fixed_parts) {
        if (!part.empty()) {
            has_content = true;
            break;
        }
    }
    if (!has_content) {
        return {};
    }

    bool pad_start = (pat[0] != '*');
    bool pad_end = (pat.back() != '*');

    std::vector<std::string> required_grams;
    for (size_t i = 0; i < fixed_parts.size(); ++i) {
        const auto& part = fixed_parts[i];
        if (part.empty()) continue;

        std::string padded = part;
        if (pad_start && i == 0) {
            padded = "$" + padded;
        }
        if (pad_end && i == fixed_parts.size() - 1) {
            padded += "$";
        }

        for (size_t j = 0; j + k_ <= padded.size(); ++j) {
            required_grams.push_back(padded.substr(j, k_));
        }
    }

    roaring::Roaring candidates;
    bool first = true;

    for (const auto& gram : required_grams) {
        auto opt = kgram_lsm_.Get(gram);
        if (!opt) {
            return {};
        }
        if (first) {
            candidates = *opt;
            first = false;
        } else {
            candidates &= *opt;
        }
    }

    if (first) {
        for (uint32_t tid = 0; tid < next_term_id_; ++tid) {
            candidates.add(tid);
        }
    }

    std::string regex_str;
    for (size_t i = 0; i < fixed_parts.size(); ++i) {
        if (i > 0) regex_str += ".*";

        std::string escaped = std::regex_replace(
            fixed_parts[i],
            std::regex(R"([\.\^\$\|\(\)\[\]\{\}\*\?\+\\/])"),
            R"(\$&)"
        );
        regex_str += escaped;
    }
    if (!pat.empty() && pat.back() == '*') {
        regex_str += ".*";
    }

    std::regex re(regex_str);

    std::vector<std::string> matching_terms;
    for (uint32_t tid : candidates) {
        if (tid >= id_to_term_.size()) continue;
        const std::string& term = id_to_term_[tid];
        if (std::regex_match(term, re)) {
            matching_terms.push_back(term);
        }
    }

    std::vector<std::pair<uint32_t, uint32_t>> all_positions;
    for (const auto& term : matching_terms) {
        auto opt = lsm_.Get(term);
        if (!opt) continue;
        all_positions.insert(all_positions.end(), opt->begin(), opt->end());
    }

    std::sort(all_positions.begin(), all_positions.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });

    return all_positions;
}