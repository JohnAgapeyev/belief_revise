#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <iterator>
#include <utility>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cassert>
#include <variant>
#include "file.h"

std::pair<type_format, std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>>> read_file(const char *path) noexcept {
    std::ifstream file{path};

    if (!file) {
        std::cerr << "Unable to open file " << strerror(errno) << "\n";
        std::cerr << path << "\n";
        return {};
    }

    std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> output{};
    bool problem_found = false;
    type_format input_type;

    for (std::string line; std::getline(file, line);) {
        //Ignore empty lines
        if (line.empty()) {
            continue;
        }
        //Ignore comment lines
        if (line.front() == 'c') {
            continue;
        }
        //Parse problem line
        if (line.front() == 'p') {
            std::istringstream iss{std::move(line)};
            std::string dummy;
            std::string format;

            //Ignore the p token for the problem line
            iss >> dummy;
            iss >> format;

            if (format.find("cnf") != std::string::npos) {
                input_type = type_format::CNF;
            } else if (format.find("dnf") != std::string::npos) {
                input_type = type_format::DNF;
            } else if (format.find("raw") != std::string::npos) {
                input_type = type_format::RAW;
            } else {
                //Unknown data format
                std::cerr << "Unknown data format\n";
                return {};
            }
            problem_found = true;
            continue;
        }
        //The first non-comment line is not a problem statement, return error
        if (!problem_found) {
            std::cerr << "First non-comment line was not a problem statement\n";
            return {};
        }
        switch(input_type) {
            case type_format::CNF:
                [[fallthrough]];
            case type_format::DNF:
                {
                    std::vector<int32_t> clause_tokens;
                    output = std::vector<decltype(clause_tokens)>{};

                    std::istringstream iss{std::move(line)};

                    clause_tokens.assign(std::istream_iterator<int32_t>(iss),
                            std::istream_iterator<int32_t>());
                    clause_tokens.erase(std::remove(clause_tokens.begin(), clause_tokens.end(), 0),
                            clause_tokens.end());
                    clause_tokens.shrink_to_fit();

                    if (clause_tokens.empty()) {
                        continue;
                    }

                    std::get<std::vector<decltype(clause_tokens)>>(output).emplace_back(std::move(clause_tokens));
                }
                break;
            case type_format::RAW:
                {
                    unsigned char c;
                    std::vector<bool> state;
                    output = std::vector<decltype(state)>{};

                    std::istringstream iss{std::move(line)};

                    for (;;) {
                        iss >> c;
                        if (iss.bad() || (!isxdigit(c) && !iscntrl(c) && !isspace(c))) {
                            std::cerr << "Failed to parse hexadecimal state\n";
                            return {};
                        }
                        if (iscntrl(c) || isspace(c) || iss.eof()) {
                            break;
                        }

                        //Need to convert char for ternery to work
                        c = toupper(c);
                        int hex_value = (c >= 'A') ? (c - 'A' + 10) : (c - '0');

                        //Push the bit values in big-endian order
                        state.push_back(hex_value & 8);
                        state.push_back(hex_value & 4);
                        state.push_back(hex_value & 2);
                        state.push_back(hex_value & 1);
                    }

                    if (state.empty()) {
                        continue;
                    }

                    //Store the bit state into the variant
                    std::get<std::vector<decltype(state)>>(output).emplace_back(std::move(state));
                }
                break;
            default:
                std::cerr << "Unknown data format enum value\n";
                return {};
        }
    }

    if ((std::holds_alternative<std::vector<std::vector<int32_t>>>(output) && std::get<std::vector<std::vector<int32_t>>>(output).empty())
            || (std::holds_alternative<std::vector<std::vector<bool>>>(output) && std::get<std::vector<std::vector<bool>>>(output).empty())) {
        std::cerr << "File must not be empty\n";
        exit(EXIT_FAILURE);
    }

    return {input_type, output};
}

//This applies the distributive property to convert between DNF and CNF DIMACS formats
std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses) noexcept {
    std::vector<std::vector<int32_t>> result;

    assert(normal_clauses.size() >= 2);

    if (normal_clauses.size() == 2) {
        //Handle final case
        for (const auto& first : normal_clauses.front()) {
            for (const auto second : normal_clauses[1]) {
                result.push_back({first, second});
            }
        }

        return result;
    } else {
        //Get the result excluding the first clause
        const auto prev_result = convert_normal_forms({std::next(normal_clauses.begin()), normal_clauses.end()});
        for (const auto term : normal_clauses.front()) {
            for (const auto& clause : prev_result) {
                //Add the first level term to the existing conversion
                std::vector<int32_t> extended_result{clause};
                extended_result.emplace_back(term);
                result.emplace_back(std::move(extended_result));
            }
        }
        for (auto& clause : result) {
            //Sort the result in variable order
            std::sort(clause.begin(), clause.end(), [](const auto lhs, const auto rhs){return std::abs(lhs) < std::abs(rhs);});
        }
        return result;
    }
}

//Converts the input bits to DNF format - It's a 1-to-1 mapping
std::vector<std::vector<int32_t>> convert_raw(const std::vector<std::vector<bool>>& input_bits) noexcept {
    std::vector<std::vector<int32_t>> output;
    output.reserve(input_bits.size());

    for (const auto& clause : input_bits) {
        std::vector<int32_t> converted_form;

        for (unsigned long i = 0; i < clause.size(); ++i) {
            converted_form.push_back((clause[i]) ? i + 1 : -(i + 1));
        }
        output.emplace_back(std::move(converted_form));
    }

    return output;
}
