#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "../lsm/lsm.h"
#include "../common/roaring/roaring.hh"

class WildCardSearch {
public:
    explicit WildCardSearch(size_t memtable_size, size_t growth_factor);

    uint32_t add_document(const std::string& text);

    std::vector<std::pair<uint32_t, uint32_t>> search(const std::string& pattern);

private:
    // Поиск по термину → список (doc_id, position)
    LSM<std::vector<std::pair<uint32_t, uint32_t>>> lsm_;

    // Поиск по k-граммам → term_id
    LSM<roaring::Roaring> kgram_lsm_;

    uint32_t next_doc_id_  = 0;
    uint32_t next_term_id_ = 0;

    std::unordered_map<std::string, uint32_t> term_to_id_;
    std::vector<std::string> id_to_term_;

    std::vector<std::string> documents_;

    static constexpr size_t k_ = 3;
};