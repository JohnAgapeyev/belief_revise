#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

//Wrapper function
std::string get_user_input() noexcept;

//Wrapper function
std::string get_formula_input() noexcept;

//The main function of interactive mode
std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> run_interactive_mode() noexcept;

//Converts an infix expression into a postfix one using the shunting yard algorithm
std::string shunting_yard(const std::string& input) noexcept;

//Evaluates the boolean equation based on provided assignments; used for truth table conversion
bool evaulate_expression(const std::vector<std::string>& tokens, const std::vector<bool>& assignments) noexcept;

//Wrapper function to get the total number of variables involved
int32_t get_max_variable_num(const std::vector<std::string>& tokens) noexcept;

//Convert the tokens of an equation into a DNF vector
std::vector<std::vector<int32_t>> get_dnf_from_equation(const std::vector<std::string>& tokens) noexcept;

#endif

