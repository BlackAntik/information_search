#include "date_index.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <stack>

namespace {

void parse_date(const std::string& s, int& year, int& month, int& day) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') {
        throw std::runtime_error("Invalid date format: " + s + " (expected YYYY-MM-DD)");
    }
    year  = std::stoi(s.substr(0, 4));
    month = std::stoi(s.substr(5, 2));
    day   = std::stoi(s.substr(8, 2));
}

int32_t days_from_epoch(int year, int month, int day) {
    year -= (month <= 2);
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace

DateIndex::DateIndex(size_t memtable_size, size_t growth_factor)
    : index_(memtable_size, growth_factor) {}

int32_t DateIndex::date_to_int(const std::string& date_str) {
    int y, m, d;
    parse_date(date_str, y, m, d);
    return days_from_epoch(y, m, d);
}

uint32_t DateIndex::add_document(const std::string& text,
                                  const std::string& begin_date,
                                  const std::string& end_date) {
    uint32_t doc_id = index_.add_document(text);
    next_doc_id_ = doc_id + 1;

    int32_t begin_val = date_to_int(begin_date);
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        if ((begin_val >> bit) & 1) {
            index_.add_term("b" + std::to_string(bit), doc_id);
        }
    }

    int32_t end_val = end_date.empty() ? MAX_DATE_VAL : date_to_int(end_date);
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        if ((end_val >> bit) & 1) {
            index_.add_term("e" + std::to_string(bit), doc_id);
        }
    }

    return doc_id;
}

roaring::Roaring DateIndex::get_bit_slice(const std::string& term) {
    return index_.get_term_bitmap(term);
}

roaring::Roaring DateIndex::all_docs() {
    roaring::Roaring full;
    full.addRange(0ULL, static_cast<uint64_t>(next_doc_id_));
    return full;
}

roaring::Roaring DateIndex::bsi_geq(const std::string& prefix, int32_t lo) {
    roaring::Roaring gt;
    roaring::Roaring eq = all_docs();

    for (int i = NUM_BITS - 1; i >= 0; --i) {
        roaring::Roaring bit_i = get_bit_slice(prefix + std::to_string(i));
        int lo_bit = (lo >> i) & 1;

        if (lo_bit == 1) {
            eq &= bit_i;
        } else {
            gt |= (eq & bit_i);
            roaring::Roaring not_bit_i = all_docs() ^ bit_i;
            eq &= not_bit_i;
        }
    }

    return gt | eq;
}

roaring::Roaring DateIndex::bsi_leq(const std::string& prefix, int32_t hi) {
    roaring::Roaring lt;
    roaring::Roaring eq = all_docs();

    for (int i = NUM_BITS - 1; i >= 0; --i) {
        roaring::Roaring bit_i = get_bit_slice(prefix + std::to_string(i));
        int hi_bit = (hi >> i) & 1;

        if (hi_bit == 0) {
            roaring::Roaring not_bit_i = all_docs() ^ bit_i;
            eq &= not_bit_i;
        } else {
            roaring::Roaring not_bit_i = all_docs() ^ bit_i;
            lt |= (eq & not_bit_i);
            eq &= bit_i;
        }
    }

    return lt | eq;
}

roaring::Roaring DateIndex::bsi_range(const std::string& prefix, int32_t lo, int32_t hi) {
    return bsi_geq(prefix, lo) & bsi_leq(prefix, hi);
}

std::vector<uint32_t> DateIndex::search_valid_in_range(const std::string& lo_date,
                                                        const std::string& hi_date) {
    int32_t lo = date_to_int(lo_date);
    int32_t hi = date_to_int(hi_date);

    roaring::Roaring result = bsi_leq("b", hi) & bsi_geq("e", lo);

    std::vector<uint32_t> docs;
    result.iterate(
        [](uint32_t value, void* ptr) -> bool {
            static_cast<std::vector<uint32_t>*>(ptr)->push_back(value);
            return true;
        },
        &docs);
    return docs;
}

std::vector<uint32_t> DateIndex::search_created_in_range(const std::string& lo_date,
                                                          const std::string& hi_date) {
    int32_t lo = date_to_int(lo_date);
    int32_t hi = date_to_int(hi_date);

    roaring::Roaring result = bsi_range("b", lo, hi);

    std::vector<uint32_t> docs;
    result.iterate(
        [](uint32_t value, void* ptr) -> bool {
            static_cast<std::vector<uint32_t>*>(ptr)->push_back(value);
            return true;
        },
        &docs);
    return docs;
}

std::pair<std::string, std::string> DateIndex::parse_date_range_args(const std::string& s, size_t start) {
    if (start >= s.size() || s[start] != '(') {
        throw std::runtime_error("Expected '(' after date function");
    }
    size_t comma = s.find(',', start);
    if (comma == std::string::npos) {
        throw std::runtime_error("Expected ',' in date function arguments");
    }
    size_t rparen = s.find(')', comma);
    if (rparen == std::string::npos) {
        throw std::runtime_error("Expected ')' in date function arguments");
    }
    std::string date1 = trim(s.substr(start + 1, comma - start - 1));
    std::string date2 = trim(s.substr(comma + 1, rparen - comma - 1));
    return {date1, date2};
}

std::vector<uint32_t> DateIndex::query(const std::string& formula) {
    if (formula.empty() || std::all_of(formula.begin(), formula.end(),
                                        [](unsigned char c) { return std::isspace(c); })) {
        return {};
    }
    auto bitmap = evaluate_formula(formula);
    std::vector<uint32_t> docs;
    bitmap.iterate(
        [](uint32_t value, void* ptr) -> bool {
            static_cast<std::vector<uint32_t>*>(ptr)->push_back(value);
            return true;
        },
        &docs);
    return docs;
}

roaring::Roaring DateIndex::evaluate_formula(const std::string& formula) {
    struct ExtToken {
        enum Type { WORD, AND_OP, OR_OP, NOT_OP, LPAREN, RPAREN, BITMAP };
        Type type;
        std::string value;
        roaring::Roaring bitmap;
    };

    std::string lower = to_lower(formula);
    std::vector<ExtToken> tokens;
    size_t i = 0;

    while (i < formula.size()) {
        if (std::isspace(static_cast<unsigned char>(formula[i]))) {
            ++i;
            continue;
        }

        if (i + 10 <= lower.size() && lower.substr(i, 10) == "date_range") {
            size_t j = i + 10;
            while (j < formula.size() && std::isspace(static_cast<unsigned char>(formula[j]))) ++j;
            if (j < formula.size() && formula[j] == '(') {
                auto [d1, d2] = parse_date_range_args(formula, j);
                size_t rparen = formula.find(')', j);
                i = rparen + 1;

                int32_t lo = date_to_int(d1);
                int32_t hi = date_to_int(d2);
                roaring::Roaring bm = bsi_leq("b", hi) & bsi_geq("e", lo);

                tokens.push_back({ExtToken::BITMAP, "", std::move(bm)});
                continue;
            }
        }

        if (i + 10 <= lower.size() && lower.substr(i, 10) == "created_in") {
            size_t j = i + 10;
            while (j < formula.size() && std::isspace(static_cast<unsigned char>(formula[j]))) ++j;
            if (j < formula.size() && formula[j] == '(') {
                auto [d1, d2] = parse_date_range_args(formula, j);
                size_t rparen = formula.find(')', j);
                i = rparen + 1;

                int32_t lo = date_to_int(d1);
                int32_t hi = date_to_int(d2);
                roaring::Roaring bm = bsi_range("b", lo, hi);

                tokens.push_back({ExtToken::BITMAP, "", std::move(bm)});
                continue;
            }
        }

        if (i + 3 <= lower.size() && lower.substr(i, 3) == "and" &&
            (i + 3 == lower.size() || !std::isalnum(static_cast<unsigned char>(lower[i + 3])))) {
            tokens.push_back({ExtToken::AND_OP, "", {}});
            i += 3;
            continue;
        }
        if (i + 2 <= lower.size() && lower.substr(i, 2) == "or" &&
            (i + 2 == lower.size() || !std::isalnum(static_cast<unsigned char>(lower[i + 2])))) {
            tokens.push_back({ExtToken::OR_OP, "", {}});
            i += 2;
            continue;
        }
        if (i + 3 <= lower.size() && lower.substr(i, 3) == "not" &&
            (i + 3 == lower.size() || !std::isalnum(static_cast<unsigned char>(lower[i + 3])))) {
            tokens.push_back({ExtToken::NOT_OP, "", {}});
            i += 3;
            continue;
        }
        if (formula[i] == '(') {
            tokens.push_back({ExtToken::LPAREN, "", {}});
            ++i;
            continue;
        }
        if (formula[i] == ')') {
            tokens.push_back({ExtToken::RPAREN, "", {}});
            ++i;
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(lower[i]))) {
            size_t start = i;
            while (i < lower.size() && std::isalnum(static_cast<unsigned char>(lower[i]))) ++i;
            std::string word = lower.substr(start, i - start);
            tokens.push_back({ExtToken::WORD, word, {}});
            continue;
        }

        throw std::runtime_error("Invalid token at position " + std::to_string(i));
    }

    auto ext_precedence = [](ExtToken::Type t) -> int {
        if (t == ExtToken::NOT_OP) return 3;
        if (t == ExtToken::AND_OP) return 2;
        if (t == ExtToken::OR_OP) return 1;
        return 0;
    };

    std::vector<ExtToken> output;
    std::stack<ExtToken> op_stack;

    for (auto& token : tokens) {
        if (token.type == ExtToken::WORD || token.type == ExtToken::BITMAP) {
            output.push_back(std::move(token));
        } else if (token.type == ExtToken::LPAREN) {
            op_stack.push(std::move(token));
        } else if (token.type == ExtToken::RPAREN) {
            while (!op_stack.empty() && op_stack.top().type != ExtToken::LPAREN) {
                output.push_back(std::move(op_stack.top()));
                op_stack.pop();
            }
            if (op_stack.empty()) throw std::runtime_error("Mismatched parentheses");
            op_stack.pop();
        } else {
            while (!op_stack.empty() && op_stack.top().type != ExtToken::LPAREN &&
                   ext_precedence(op_stack.top().type) >= ext_precedence(token.type)) {
                output.push_back(std::move(op_stack.top()));
                op_stack.pop();
            }
            op_stack.push(std::move(token));
        }
    }
    while (!op_stack.empty()) {
        if (op_stack.top().type == ExtToken::LPAREN) throw std::runtime_error("Mismatched parentheses");
        output.push_back(std::move(op_stack.top()));
        op_stack.pop();
    }

    if (output.empty()) return roaring::Roaring();

    std::stack<roaring::Roaring> stk;
    for (auto& token : output) {
        if (token.type == ExtToken::WORD) {
            auto docs = index_.query(token.value);
            roaring::Roaring bm;
            for (auto d : docs) bm.add(d);
            stk.push(std::move(bm));
        } else if (token.type == ExtToken::BITMAP) {
            stk.push(std::move(token.bitmap));
        } else if (token.type == ExtToken::NOT_OP) {
            if (stk.empty()) throw std::runtime_error("NOT missing operand");
            auto b = std::move(stk.top());
            stk.pop();
            stk.push(all_docs() ^ b);
        } else if (token.type == ExtToken::AND_OP) {
            if (stk.size() < 2) throw std::runtime_error("AND missing operand");
            auto b2 = std::move(stk.top()); stk.pop();
            auto b1 = std::move(stk.top()); stk.pop();
            b1 &= b2;
            stk.push(std::move(b1));
        } else if (token.type == ExtToken::OR_OP) {
            if (stk.size() < 2) throw std::runtime_error("OR missing operand");
            auto b2 = std::move(stk.top()); stk.pop();
            auto b1 = std::move(stk.top()); stk.pop();
            b1 |= b2;
            stk.push(std::move(b1));
        }
    }

    if (stk.size() != 1) throw std::runtime_error("Invalid expression");
    return std::move(stk.top());
}
