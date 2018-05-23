#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

std::string get_user_input() noexcept;

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> run_interactive_mode() noexcept;

std::string shunting_yard(const std::string& input) noexcept;

#endif

