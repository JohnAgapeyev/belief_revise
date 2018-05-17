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
#include "file.h"

std::pair<std::vector<std::vector<bool>>, std::vector<std::vector<int32_t>>> read_file(const char *path) {
    std::ifstream file{path};

    if (!file) {
        std::cerr << "Unable to open file " << strerror(errno) << "\n";
        std::cerr << path << "\n";
        return {};
    }

    std::vector<std::vector<int32_t>> clause_list;
    std::vector<std::vector<bool>> belief_state;

    bool is_formula = false;

    for (std::string line; std::getline(file, line);) {
        //Ignore comment lines
        if (line.front() == '#') {
            continue;
        }
        //We've hit the formula delimiter
        if (tolower(line.front()) == 's') {
            is_formula = true;
            continue;
        }
        if (!is_formula) {
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

            belief_state.emplace_back(std::move(state));
        } else {
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

            clause_list.emplace_back(std::move(clause_tokens));
        }
    }

    if (belief_state.empty() || clause_list.empty()) {
        std::cerr << "File must contain both an initial belief state, and a formula\n";
        return {};
    }

    return {belief_state, clause_list};
}
