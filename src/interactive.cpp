#include <string>
#include <iostream>
#include <sstream>
#include <stack>
#include <queue>
#include <cstdlib>
#include "interactive.h"

std::string get_user_input() noexcept {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> run_interactive_mode() noexcept {
    std::cout << "Entering initial belief states:\n";
bad_format:
    std::cout << "Please select your belief input format:\n";
    std::cout << "c - Conjunctive Normal Form, aka CNF\n";
    std::cout << "d - Disjunctive Normal Form, aka DNF\n";
    std::cout << "[c/d]? ";

    auto input = get_user_input();
    if (input.empty() || (input.front() != 'c' && input.front() != 'd')) {
        std::cout << "Invalid selection\n";
        goto bad_format;
    }

    if (input.front() == 'c') {
        std::cout << "You have selected Conjunctive Normal Form, aka CNF\n";
    } else {
        std::cout << "You have selected Disjunctive Normal Form, aka DNF\n";
    }

bad_equation:
    std::cout << "Please enter your belief equation in the selected format or \'h\' to display a help message detailing required formatting rules\n";
    input = get_user_input();
    if (input.empty()) {
        std::cout << "Invalid selection\n";
        goto bad_equation;
    }
    if (input.front() == 'h') {
        std::cout << "Lorem Ipsum Dolor Sit Amet\n";
        goto bad_equation;
    }

    const auto formatted = shunting_yard(input);

    std::cout << formatted << "\n";


    return {};
}

std::string shunting_yard(const std::string& input) noexcept {
    std::istringstream iss{input};

    std::stack<std::string> operator_stack;

    std::string token;

    std::stringstream output;

    while (iss >> token) {
        int32_t numeric_token;
        if ((numeric_token = std::strtol(token.c_str(), nullptr, 10)) == 0 || errno == ERANGE) {
            //Bad int conversion so it must be an operator or parenthesis
            if (token.find("and") != std::string::npos) {
                operator_stack.push("and");
            } else if (token.find("or") != std::string::npos) {
                operator_stack.push("or");
            } else if (token.find("not") != std::string::npos) {
                iss.clear();
                int32_t tmp;
                if (iss >> tmp) {
                    //variable_queue.push(tmp * -1);
                    output << (tmp * -1) << ' ';
                } else {
                    std::cout << "Error attempting to negate provided term\n";
                    return "";
                }
            } else {
                //Not a number or an operation
                if (token.front() == '(') {
                    //New equation
                    operator_stack.push("(");
                } else if (token.front() == ')') {
                    //End of current equation
                    if (operator_stack.empty()) {
                        std::cout << "Invalid equation formatting\n";
                        return "";
                    }
                    while(operator_stack.top().front() != '(') {
                        output << operator_stack.top() << ' ';
                        operator_stack.pop();

                        if (operator_stack.empty()) {
                            std::cout << "Invalid equation formatting\n";
                            return "";
                        }
                    }
                    //Pop off the '(' character
                    operator_stack.pop();
                } else {
                    std::cout << "Unknown token found\n";
                    return "";
                }
            }
        } else {
            //Successful int conversion
            //variable_queue.push(numeric_token);
            output << numeric_token << ' ';
        }
    }

    //Add any remaining operations to the string
    while(!operator_stack.empty()) {
        output << operator_stack.top() << ' ';
        operator_stack.pop();
    }

    return output.str();
}

