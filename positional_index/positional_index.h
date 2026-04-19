#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>
#include "../lsm/lsm.h"

using PostingList = std::vector<std::pair<uint32_t, uint32_t>>;

class PositionalIndex {
public:
    PositionalIndex(size_t memtable_size, size_t growth_factor);

    uint32_t add_document(const std::string& text);

    std::vector<std::pair<uint32_t, uint32_t>> phrase_query(const std::string& phrase);

    std::vector<std::string> tokenize(const std::string& text);

private:
    LSM<PostingList> lsm_;
    uint32_t next_doc_id_ = 0;

    std::string to_lower(const std::string& s);
    PostingList get_posting_list(const std::string& term);
    std::vector<std::pair<uint32_t, uint32_t>> intersect_phrase(const std::vector<PostingList>& postings);
};
