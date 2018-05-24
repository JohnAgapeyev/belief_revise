#include <string>
#include <iostream>
#include <sstream>
#include <deque>
#include <queue>
#include <cstdlib>
#include <unordered_set>
#include <iterator>
#include <bitset>
#include "interactive.h"
#include "file.h"

std::string get_user_input() noexcept {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string get_formula_input() noexcept {
bad_equation:
    std::cout << "Please enter your equation or \'h\' to display a help message detailing required formatting rules\n";
    const auto input = get_user_input();
    if (input.empty()) {
        std::cout << "Invalid selection\n";
        goto bad_equation;
    }
    if (input.front() == 'h') {
        std::cout << "Lorem Ipsum Dolor Sit Amet\n";
        goto bad_equation;
    }
    return input;
}

int32_t get_max_variable_num(const std::vector<std::string>& tokens) noexcept {
    int32_t max_variable = INT32_MIN;
    for (const auto& str : tokens) {
        int32_t parsed_variable;
        if ((parsed_variable = std::strtol(str.c_str(), nullptr, 10)) == 0 || errno == ERANGE) {
            //Failed to parse, can't be the token I want
            continue;
        }
        max_variable = std::max(max_variable, std::abs(parsed_variable));
    }
    return max_variable;
}

std::vector<std::vector<int32_t>> get_dnf_from_equation(const std::vector<std::string>& tokens) noexcept {
    const auto max_variable = get_max_variable_num(tokens);

    //Fill the converted state to have variable_count falses
    std::vector<bool> converted_state{static_cast<unsigned long>(max_variable), false, std::allocator<bool>()};

    std::vector<std::vector<int32_t>> output_dnf;

    for (uint64_t mask = 0; mask < (1ull << max_variable); ++mask) {
        std::bitset<64> bs{mask};

        for (long i = 0; i < max_variable; ++i) {
            converted_state[i] = bs[i];
        }

        if (evaulate_expression(tokens, converted_state)) {
            //Convert truth evaluation to DNF format using Sum of Products
            std::vector<int32_t> dnf_clause;
            for (long i = 0; i < max_variable; ++i) {
                int32_t term = i + 1;
                if (!converted_state[i]) {
                    term *= -1;
                }
                dnf_clause.push_back(term);
            }
            output_dnf.emplace_back(std::move(dnf_clause));
        }
    }
    return output_dnf;
}

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> run_interactive_mode() noexcept {
    std::cout << "Entering initial belief states:\n";

    std::stringstream ss{shunting_yard(get_formula_input())};

    std::vector<std::string> tokens{std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

    const auto raw_belief_states = convert_dnf_to_raw(get_dnf_from_equation(tokens));

    std::cout << "Initial belief states have been generated\n";
    std::cout << "Entering revision formula:\n";

    ss = std::stringstream{shunting_yard(get_formula_input())};

    tokens = std::vector<std::string>{std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

    const auto formula_cnf = convert_normal_forms(get_dnf_from_equation(tokens));

    return {raw_belief_states, formula_cnf};
}

std::string shunting_yard(const std::string& input) noexcept {
    std::istringstream iss{input};

    std::deque<std::string> operator_stack;

    std::string token;

    std::stringstream output;

    while (iss >> token) {
reparse:
        int32_t numeric_token;
        char *num_parse_index;
        if ((numeric_token = std::strtol(token.c_str(), &num_parse_index, 10)) == 0 || errno == ERANGE) {
            //Bad int conversion so it must be an operator or parenthesis
            if (token.find("and") == 0) {
                operator_stack.push_front("and");
            } else if (token.find("or") == 0) {
                operator_stack.push_front("or");
            } else if (token.find("not") == 0) {
                iss.clear();
                int32_t negated_token;
                if (iss >> negated_token) {
                    output << (negated_token * -1) << ' ';
                } else {
                    std::cout << "Error attempting to negate provided term\n";
                    return "";
                }
            } else {
                //Not a number or an operation
                if (token.front() == '(') {
                    //New equation
                    operator_stack.push_front("(");

                    if (token.size() > 1 && !std::isspace(token[1])) {
                        //Pop off the first character and reparse the token
                        token.assign(token, 1, std::string::npos);
                        goto reparse;
                    }
                } else if (token.front() == ')') {
                    //End of current equation
                    if (operator_stack.empty()) {
                        std::cout << "Invalid equation formatting\n";
                        return "";
                    }
                    while(operator_stack.front().front() != '(') {
                        output << operator_stack.front() << ' ';
                        operator_stack.pop_front();

                        if (operator_stack.empty()) {
                            std::cout << "Invalid equation formatting\n";
                            return "";
                        }
                    }
                    //Pop off the '(' character
                    operator_stack.pop_front();

                    //Another closing bracket was found
                    if (token.size() > 1 && token[1] == ')') {
                        //Pop off the first character and reparse the token
                        token.assign(token, 1, std::string::npos);
                        goto reparse;
                    }
                } else {
                    std::cout << "Unknown token found\n";
                    return "";
                }
            }
        } else {
            //Successful int conversion
            output << numeric_token << ' ';

            //Number literal is touching a close bracket, reparse the bracket
            if (num_parse_index && *num_parse_index == ')') {
                token = std::string(num_parse_index);
                goto reparse;
            }
        }
    }

    //Add any remaining operations to the string
    while(!operator_stack.empty()) {
        output << operator_stack.front() << ' ';
        operator_stack.pop_front();
    }

    std::string out_str = output.str();

    if (out_str.empty()) {
        std::cout << "Equation parser returned empty string\n";
        return "";
    }

    //Remove trailing whitespace
    while(std::isspace(out_str.back())) {
        out_str.pop_back();
    }

    return out_str;
}

//Returns whether the current variable assignment satisfies the postfix equation string
bool evaulate_expression(const std::vector<std::string>& tokens, const std::vector<bool>& assignments) noexcept {
    std::deque<bool> variable_outputs;

    for (const auto& str : tokens) {
        int32_t numeric_token;
        if ((numeric_token = std::strtol(str.c_str(), nullptr, 10)) == 0 || errno == ERANGE) {
            //Token is not a number

            bool first = variable_outputs.front();
            variable_outputs.pop_front();
            bool second = variable_outputs.front();
            variable_outputs.pop_front();

            if (str.find("and") == 0) {
                variable_outputs.push_front(first && second);
            } else if (str.find("or") == 0) {
                variable_outputs.push_front(first || second);
            }
        } else {
            //Token is a number
            variable_outputs.push_front(assignments[std::abs(numeric_token) - 1]);
        }
    }

    return variable_outputs.front();
}

