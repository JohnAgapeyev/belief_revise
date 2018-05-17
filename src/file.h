#ifndef FILE_H
#define FILE_H

#include <vector>
#include <cstdint>
#include <utility>

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> read_file(const char *path);

#endif
