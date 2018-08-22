#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "utils.h"

std::vector<std::vector<bool>> convert_to_bool(const std::vector<std::vector<int32_t>>& state) noexcept {
    std::vector<std::vector<bool>> output;

    for (const auto& clause : state) {
        const auto max_term = std::abs(*std::max_element(clause.cbegin(), clause.cend(), [](const auto& lhs, const auto& rhs){return std::abs(lhs) < std::abs(rhs);}));
        std::vector<bool> converted_term{static_cast<unsigned long>(max_term), false, std::allocator<bool>()};
        for (const auto term : clause) {
            converted_term[std::abs(term) - 1] = (term > 0);
        }
        output.emplace_back(std::move(converted_term));
    }

    return output;
}

std::vector<std::vector<int32_t>> convert_to_num(const std::vector<std::vector<bool>>& state) noexcept {
    std::vector<std::vector<int32_t>> output;

    for (const auto& clause : state) {
        std::vector<int32_t> converted_term;
        for (unsigned long i = 0; i < clause.size(); ++i) {
            int32_t term = i + 1;
            if (!clause[i]) {
                term *= -1;
            }
            converted_term.emplace_back(term);
        }
        output.emplace_back(std::move(converted_term));
    }

    return output;
}

std::string print_formula_dnf(const std::vector<std::vector<int32_t>>& formula) noexcept {
    std::stringstream ss;
    for (const auto& clause : formula) {
        ss << '(';
        for (const auto term : clause) {
            if (term < 0) {
                ss << "not ";
            }
            ss << std::abs(term);
            if (term != clause.back()) {
                ss << " and ";
            }
        }
        ss << ')';
        if (clause != formula.back()) {
            ss << " or ";
        }
    }
    ss << std::endl;
    std::cout << ss.str();
    return ss.str();
}

