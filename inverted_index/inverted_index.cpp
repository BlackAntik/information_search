#include "inverted_index.h"
#include <algorithm>
#include <cctype>
#include <stack>
#include <stdexcept>

namespace {
    std::string to_lower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return result;
    }
    
    int precedence(InvertedIndex::TokenType t) {
        if (t == InvertedIndex::TokenType::NOT_OP) return 3;
        if (t == InvertedIndex::TokenType::AND_OP) return 2;
        if (t == InvertedIndex::TokenType::OR_OP) return 1;
        return 0;
    }
}



InvertedIndex::InvertedIndex(size_t memtable_size, size_t growth_factor)
    : lsm_(memtable_size, growth_factor, [](const std::optional<roaring::Roaring>& first, const std::optional<roaring::Roaring>& second){
        if (!first || !second) {
            throw std::runtime_error("Unexpected input values");
        }
        return *first | *second;
    }) {

    // не знаю готовой библы поэтому попросил ИИ закинуть какие-то дефолтные английские стоп-слова
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

std::string InvertedIndex::stem(const std::string& word) {
    if (word.empty()) return "";
    std::wstring wword(word.begin(), word.end());
    english_stemmer_(wword);
    return std::string(wword.begin(), wword.end());
}

std::unordered_set<std::string> InvertedIndex::preprocess(const std::string& text) {
    std::unordered_set<std::string> result;
    const std::string lower = to_lower(text);
    size_t i = 0;
    while (i < lower.size()) {
        if (!std::isalpha(lower[i])) {
            ++i;
            continue;
        }
        const size_t start = i;
        while (i < lower.size() && std::isalpha(lower[i])) ++i;
        std::string word = lower.substr(start, i - start);
        if (stop_words_.count(word)) continue;
        std::string stemmed = stem(word);
        if (!stemmed.empty()) result.insert(stemmed);
    }
    return result;
}

uint32_t InvertedIndex::add_document(const std::string& text) {
    uint32_t doc_id = next_doc_id_++;
    auto words = preprocess(text);
    for (const auto& word : words) {
        roaring::Roaring bitmap;
        bitmap.add(doc_id);
        lsm_.Put(word, bitmap);
    }
    return doc_id;
}

std::vector<InvertedIndex::Token> InvertedIndex::tokenize(const std::string& query) {
    std::vector<Token> tokens;
    std::string lower = to_lower(query);
    size_t i = 0;
    while (i < lower.size()) {
        if (std::isspace(lower[i])) {
            ++i;
            continue;
        }
        if (lower.substr(i, 3) == "and" && (i + 3 == lower.size() || !std::isalnum(lower[i + 3]))) {
            tokens.push_back({TokenType::AND_OP, ""});
            i += 3;
            continue;
        }
        if (lower.substr(i, 2) == "or" && (i + 2 == lower.size() || !std::isalnum(lower[i + 2]))) {
            tokens.push_back({TokenType::OR_OP, ""});
            i += 2;
            continue;
        }
        if (lower.substr(i, 3) == "not" && (i + 3 == lower.size() || !std::isalnum(lower[i + 3]))) {
            tokens.push_back({TokenType::NOT_OP, ""});
            i += 3;
            continue;
        }
        if (lower[i] == '(') {
            tokens.push_back({TokenType::LPAREN, ""});
            ++i;
            continue;
        }
        if (lower[i] == ')') {
            tokens.push_back({TokenType::RPAREN, ""});
            ++i;
            continue;
        }
        if (std::isalpha(lower[i])) {
            size_t start = i;
            while (i < lower.size() && std::isalnum(lower[i])) ++i;
            std::string word = lower.substr(start, i - start);
            tokens.push_back({TokenType::WORD, word});
            continue;
        }
        throw std::runtime_error("Invalid token at position " + std::to_string(i));
    }
    return tokens;
}

std::vector<InvertedIndex::Token> InvertedIndex::to_rpn(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    std::stack<TokenType> op_stack;
    for (const auto& token : tokens) {
        if (token.type == TokenType::WORD) {
            output.push_back(token);
        } else if (token.type == TokenType::LPAREN) {
            op_stack.push(token.type);
        } else if (token.type == TokenType::RPAREN) {
            while (!op_stack.empty() && op_stack.top() != TokenType::LPAREN) {
                output.push_back({op_stack.top(), ""});
                op_stack.pop();
            }
            if (op_stack.empty()) {
                throw std::runtime_error("Mismatched parentheses");
            }
            op_stack.pop();
        } else {
            while (!op_stack.empty() && op_stack.top() != TokenType::LPAREN &&
                   precedence(op_stack.top()) >= precedence(token.type)) {
                output.push_back({op_stack.top(), ""});
                op_stack.pop();
            }
            op_stack.push(token.type);
        }
    }
    while (!op_stack.empty()) {
        if (op_stack.top() == TokenType::LPAREN) {
            throw std::runtime_error("Mismatched parentheses");
        }
        output.push_back({op_stack.top(), ""});
        op_stack.pop();
    }
    return output;
}

roaring::Roaring InvertedIndex::evaluate(const std::vector<Token>& rpn) {
    if (rpn.empty()) throw std::runtime_error("Empty expression");

    std::stack<roaring::Roaring> stk;
    for (const auto& token : rpn) {
        if (token.type == TokenType::WORD) {
            std::string w = stem(token.value);
            auto opt = lsm_.Get(w);
            roaring::Roaring b;
            if (opt.has_value()) {
                b = opt.value();
            }
            stk.push(b);
        } else if (token.type == TokenType::NOT_OP) {
            if (stk.empty()) throw std::runtime_error("Invalid expression: NOT missing operand");
            auto b = std::move(stk.top());
            stk.pop();
            roaring::Roaring full;
            full.addRange(0ULL, static_cast<uint64_t>(next_doc_id_));
            roaring::Roaring not_b = full ^ b;
            stk.push(std::move(not_b));
        } else if (token.type == TokenType::AND_OP || token.type == TokenType::OR_OP) {
            if (stk.size() < 2) throw std::runtime_error("Invalid expression: binary operator missing operand");
            auto b2 = std::move(stk.top());
            stk.pop();
            auto b1 = std::move(stk.top());
            stk.pop();
            if (token.type == TokenType::AND_OP) {
                b1 &= b2;
                stk.push(std::move(b1));
            } else if (token.type == TokenType::OR_OP) {
                b1 |= b2;
                stk.push(std::move(b1));
            }
        } else {
            throw std::runtime_error("Invalid token in RPN");
        }
    }
    if (stk.size() != 1) throw std::runtime_error("Invalid expression: leftover operands");
    return std::move(stk.top());
}

std::vector<uint32_t> InvertedIndex::query(const std::string& formula) {
    if (formula.empty() || std::all_of(formula.begin(), formula.end(), [](unsigned char c){ return std::isspace(c); })) {
        return {};
    }
    auto tokens = tokenize(formula);
    if (!tokens.empty()) {
        TokenType last = tokens.back().type;
        if (last == TokenType::AND_OP || last == TokenType::OR_OP || last == TokenType::NOT_OP) {
            throw std::runtime_error("Expression ends with operator");
        }
    }
    auto rpn = to_rpn(tokens);
    auto bitmap = evaluate(rpn);
    std::vector<uint32_t> docs;
    bitmap.iterate(
        [](uint32_t value, void *ptr) -> bool {
            auto docsPtr = static_cast<std::vector<uint32_t>*>(ptr);
            docsPtr->push_back(value);
            return true;
        },
        &docs);
    return docs;
}