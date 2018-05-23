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

std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> read_file(const char *path) noexcept;

std::pair<type_format, std::vector<std::vector<int32_t>>> read_dimacs_data(std::ifstream& ifs) noexcept;

std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses) noexcept;

#endif
