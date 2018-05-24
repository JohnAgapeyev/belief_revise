#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

std::string get_user_input() noexcept;

std::string get_formula_input() noexcept;

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> run_interactive_mode() noexcept;

std::string shunting_yard(const std::string& input) noexcept;

bool evaulate_expression(const std::vector<std::string>& tokens, const std::vector<bool>& assignments) noexcept;

int32_t get_max_variable_num(const std::vector<std::string>& tokens) noexcept;

std::vector<std::vector<int32_t>> get_dnf_from_equation(const std::vector<std::string>& tokens) noexcept;

#endif

