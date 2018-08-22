#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <cstdint>
#include <string>

std::vector<std::vector<bool>> convert_to_bool(const std::vector<std::vector<int32_t>>& state) noexcept;
std::vector<std::vector<int32_t>> convert_to_num(const std::vector<std::vector<bool>>& state) noexcept;

std::string print_formula_dnf(const std::vector<std::vector<int32_t>>& formula) noexcept;

#endif
