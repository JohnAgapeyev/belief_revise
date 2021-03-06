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
#include <bitset>
#include <unordered_set>
#include "file.h"

//Returns an enum along with a variant, where the enum says what type the variant has
//This means this same piece of code can parse all 3 accepted input formats without needing seperate functions
std::pair<type_format, std::variant<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>>> read_file(const char *path) noexcept {
    std::ifstream file{path};

    if (!file) {
        std::cerr << "Unable to open file " << strerror(errno) << "\n";
        std::cerr << path << "\n";
        return {};
    }

    std::vector<std::vector<int32_t>> output_clauses;
    std::vector<std::vector<bool>> output_bits;

    bool problem_found = false;
    type_format input_type = type_format::RAW;

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

                    std::istringstream iss{std::move(line)};

                    clause_tokens.assign(std::istream_iterator<int32_t>(iss),
                            std::istream_iterator<int32_t>());
                    clause_tokens.erase(std::remove(clause_tokens.begin(), clause_tokens.end(), 0),
                            clause_tokens.end());
                    clause_tokens.shrink_to_fit();

                    if (clause_tokens.empty()) {
                        continue;
                    }
                    output_clauses.emplace_back(std::move(clause_tokens));
                }
                break;
            case type_format::RAW:
                {
                    unsigned char c;
                    std::vector<bool> state;

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
                    output_bits.emplace_back(std::move(state));
                }
                break;
            default:
                std::cerr << "Unknown data format enum value\n";
                return {};
        }
    }

    if (input_type == type_format::RAW) {
        if (output_bits.empty()) {
            std::cerr << "File must not be empty\n";
            exit(EXIT_FAILURE);
        }
        return {input_type, output_bits};
    } else {
        if (output_clauses.empty()) {
            std::cerr << "File must not be empty\n";
            exit(EXIT_FAILURE);
        }
        return {input_type, output_clauses};
    }
}

//This applies the distributive property to convert between DNF and CNF DIMACS formats
std::vector<std::vector<int32_t>> convert_normal_forms(const std::vector<std::vector<int32_t>>& normal_clauses) noexcept {
    std::vector<std::vector<int32_t>> result;

    if (normal_clauses.empty()) {
        return normal_clauses;
    }

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

//This needs to pad for all unset variables
//So if each clause doesn't specify every variable's state, then theoretically it can be in any state
//Eg. A and B only specifies the first 2 bits, but if there are 8 variables, then those other 6 could be any state, hence the need for padding
//So padding will be necessary, unfortunately
//This will greatly slow things down if the examples are simple
std::vector<std::vector<bool>> convert_dnf_to_raw(const std::vector<std::vector<int32_t>>& clause_list) noexcept {
    std::unordered_set<std::vector<bool>> output_set;
    output_set.reserve(clause_list.size());

    int32_t variable_count = INT32_MIN;
#pragma omp parallel shared(output_set, variable_count)
    {
#pragma omp for schedule(static) reduction(max: variable_count) nowait
        for (auto it = clause_list.cbegin(); it < clause_list.cend(); ++it) {
            for (auto it2 = it->cbegin(); it2 < it->cend(); ++it2) {
                variable_count = std::max(variable_count, std::abs(*it2));
            }
        }

#pragma omp for schedule(static) nowait
        for (auto it = clause_list.cbegin(); it < clause_list.cend(); ++it) {
            //Fill the converted state to have variable_count falses
            std::vector<bool> converted_state{static_cast<unsigned long>(variable_count), false, std::allocator<bool>()};

            std::unordered_set<int32_t> variable_set;
            for (const auto term : *it) {
                variable_set.emplace(std::abs(term) - 1);

                //Fill in the bits set by the clause
                converted_state[std::abs(term) - 1] = (term > 0);
            }
            int32_t clause_max = variable_set.size();

            //Brute force pad the unused bits
            for (uint64_t mask = 0; mask < (1ull << (variable_count - clause_max)); ++mask) {
                std::bitset<64> bs{mask};

                unsigned long pos = 0;
                for (int32_t i = 0; i < variable_count; ++i) {
                    //Variable at position i is not set by the clause
                    if (!variable_set.count(i)) {
                        converted_state[i] = bs[pos++];
                    }
                }
#pragma omp critical
                output_set.emplace(converted_state);
            }
        }
    }

    std::vector<std::vector<bool>> output{std::make_move_iterator(output_set.begin()), std::make_move_iterator(output_set.end())};

    return output;
}

std::unordered_map<int32_t, unsigned long> read_pd_ordering(const char *path) noexcept {
    std::ifstream file{path};

    if (!file) {
        std::cerr << "Unable to open file " << strerror(errno) << "\n";
        std::cerr << path << "\n";
        return {};
    }

    unsigned long distance_value = 1;
    std::unordered_map<int32_t, unsigned long> orderings;
    int32_t max_variable = -1;

    for (std::string line; std::getline(file, line);) {
        //Ignore empty lines
        if (line.empty()) {
            continue;
        }

        for (const auto c : line) {
            if (!std::isdigit(c) && !std::isspace(c)) {
                std::cerr << "Line contained invalid character\n";
                return {};
            }
        }

        std::istringstream iss{std::move(line)};
        int32_t variable_num;

        while(iss >> variable_num) {
            if (variable_num <= 0) {
                std::cerr << "Variable numbers cannot be negative\n";
                return {};
            }
            orderings.emplace(variable_num, distance_value);
            max_variable = std::max(variable_num, max_variable);
        }
        ++distance_value;
    }

    for (auto i = 1; i < max_variable; ++i) {
        if (!orderings.count(i)) {
            std::cerr << "There must be no gaps in variable orderings\n";
            return {};
        }
    }

    for (auto& p : orderings) {
        p.second = max_variable - p.second;
    }

    return orderings;
}

