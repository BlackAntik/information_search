#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <optional>
#include "../lsm/lsm.h"
#include "../common/roaring/roaring.hh"
#include "../common/stemming/english_stem.h"

class InvertedIndex {
public:
    InvertedIndex(size_t memtable_size, size_t growth_factor);

    uint32_t add_document(const std::string& text);

    /// Добавить произвольный термин для указанного документа напрямую (без preprocess/стемминга).
    void add_term(const std::string& term, uint32_t doc_id);

    /// Получить bitmap для термина напрямую из LSM (без preprocess/стемминга).
    roaring::Roaring get_term_bitmap(const std::string& term);

    std::vector<uint32_t> query(const std::string& formula);

    std::unordered_set<std::string> preprocess(const std::string& text);

    enum class TokenType { WORD, AND_OP, OR_OP, NOT_OP, LPAREN, RPAREN };
    struct Token {
        TokenType type;
        std::string value;
    };

    std::vector<Token> tokenize(const std::string& query);
    std::vector<Token> to_rpn(const std::vector<Token>& tokens);

private:
    LSM<roaring::Roaring> lsm_;
    uint32_t next_doc_id_ = 0;
    std::unordered_set<std::string> stop_words_;
    stemming::english_stem<> english_stemmer_;

    std::string stem(const std::string& word);

    roaring::Roaring evaluate(const std::vector<Token>& rpn);
};