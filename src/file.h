#ifndef FILE_H
#define FILE_H

#include <vector>
#include <cstdint>
#include <utility>
#include <fstream>
#include <variant>

enum class type_format {
    CNF,
    DNF,
    RAW
};

std::pair<type_format, std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>>> read_file(const char *path) noexcept;

std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses) noexcept;
std::vector<std::vector<int32_t>> convert_raw(const std::vector<std::vector<bool>>& input_bits) noexcept;
std::vector<std::vector<bool>> convert_dnf_to_raw(const std::vector<std::vector<int32_t>>& clause_list) noexcept;

#endif
