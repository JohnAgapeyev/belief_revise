#ifndef FILE_H
#define FILE_H

#include <vector>
#include <cstdint>
#include <utility>

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> read_file(const char *path);

//This will be implemented later
std::vector<std::vector<int32_t>> read_logic_formula(const char *path);

std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses);

#endif
