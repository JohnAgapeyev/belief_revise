#ifndef FILE_H
#define FILE_H

#include <vector>
#include <cstdint>
#include <utility>
#include <fstream>
#include <variant>
#include <unordered_map>

enum class type_format {
    CNF,
    DNF,
    RAW
};

//Returns a vector of data with a enum saying what type the variant holds
std::pair<type_format, std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>>> read_file(const char *path) noexcept;

//Converts between CNF and DNF using the distributive property, hence why this function can be shared
std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses) noexcept;

//Converts between bool vectors and DNF vectors
std::vector<std::vector<int32_t>> convert_raw(const std::vector<std::vector<bool>>& input_bits) noexcept;

//Converts between DNF vectors and bool vectors
std::vector<std::vector<bool>> convert_dnf_to_raw(const std::vector<std::vector<int32_t>>& clause_list) noexcept;

std::unordered_map<int32_t, unsigned long> read_pd_ordering(const char *path) noexcept;

#endif
