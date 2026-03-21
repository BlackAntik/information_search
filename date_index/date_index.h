#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../inverted_index/inverted_index.h"
#include "../common/roaring/roaring.hh"

class DateIndex {
public:
    static constexpr int NUM_BITS = 21;
    static constexpr int32_t MAX_DATE_VAL = (1 << NUM_BITS) - 1;

    DateIndex(size_t memtable_size, size_t growth_factor);

    static int32_t date_to_int(const std::string& date_str);

    uint32_t add_document(const std::string& text,
                          const std::string& begin_date,
                          const std::string& end_date = "");

    std::vector<uint32_t> query(const std::string& formula);

    std::vector<uint32_t> search_valid_in_range(const std::string& lo_date,
                                                 const std::string& hi_date);

    std::vector<uint32_t> search_created_in_range(const std::string& lo_date,
                                                   const std::string& hi_date);

private:
    InvertedIndex index_;
    uint32_t next_doc_id_ = 0;

    roaring::Roaring bsi_geq(const std::string& prefix, int32_t lo);
    roaring::Roaring bsi_leq(const std::string& prefix, int32_t hi);
    roaring::Roaring bsi_range(const std::string& prefix, int32_t lo, int32_t hi);
    roaring::Roaring get_bit_slice(const std::string& term);
    roaring::Roaring all_docs();
    roaring::Roaring evaluate_formula(const std::string& formula);
    static std::pair<std::string, std::string> parse_date_range_args(const std::string& s, size_t start);
};
