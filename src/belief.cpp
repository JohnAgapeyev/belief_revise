#include <vector>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <climits>
#include <map>
#include <iostream>
#include <cmath>
#include <bitset>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "belief.h"

static bool satisfies(const std::bitset<64>& state, const std::vector<std::vector<int32_t>>& clause_list) noexcept {
    for (const auto& clause : clause_list) {
        bool term_false = true;
        for (const auto term : clause) {
            if (state[std::abs(term) - 1] != (term < 0)) {
                term_false = false;
                break;
            }
        }
        if (term_false) {
            return false;
        }
    }
    return true;
}

//This is an n bit brute force where n is the length of each belief
//I don't like this, but unless I settle for 1 state per formula, my only option is an ALL-SAT solver
//Which is pretty damn rare, and I can't find significant information outside of 1 or 2 papers
std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept {
#if 0
    //We need to brute force belief_length numbers of variables
    if (belief_length > 64) {
        //Undefined behaviour, and frankly, we'll never brute force 64 bits
        std::cerr << "State generation out of range\n";
        abort();
    }

    std::vector<std::vector<bool>> generated_states;

#pragma omp parallel for shared(generated_states, clause_list)
    for (uint64_t mask = 0; mask < (1ull << (belief_length)); ++mask) {
        std::bitset<64> bs{mask};

        if (!satisfies(bs, clause_list)) {
            continue;
        }

        //Set the good state
        std::vector<bool> good_state;
        good_state.reserve(belief_length);
        for (unsigned long i = 0; i < belief_length; ++i) {
            good_state.push_back(bs[i]);
        }
#pragma omp critical
        generated_states.emplace_back(std::move(good_state));
    }

    return generated_states;
#else

    const char *input_filename = ".tmp.input";
    const char *output_filename = ".tmp.output";

    creat(input_filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    creat(output_filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    {
        std::ofstream ofs{input_filename, std::ios_base::out | std::ios_base::trunc};
        if (!ofs) {
            std::cerr << "Unable to open output file\n";
            exit(EXIT_FAILURE);
        }

        for (const auto& clause : clause_list) {
            std::copy(clause.cbegin(), clause.cend(), std::ostream_iterator<int32_t>(ofs, " "));
            ofs << "0\n";
        }
    }

    system("./minisat_all/bdd_minisat_all_release .tmp.input .tmp.output");

    std::ifstream ifs{output_filename};
    if (!ifs) {
        std::cerr << "Unable to open results file\n";
        exit(EXIT_FAILURE);
    }

    std::vector<std::vector<int32_t>> output_states;

    for (std::string line; std::getline(ifs, line);) {
        std::istringstream iss{std::move(line)};

        std::vector<int32_t> clause_tokens;

        clause_tokens.assign(std::istream_iterator<int32_t>(iss),
                std::istream_iterator<int32_t>());
        clause_tokens.erase(std::remove(clause_tokens.begin(), clause_tokens.end(), 0),
                clause_tokens.end());
        clause_tokens.shrink_to_fit();

        if (clause_tokens.empty()) {
            continue;
        }

        output_states.emplace_back(std::move(clause_tokens));
    }

    std::vector<std::vector<bool>> generated_states;

    for (const auto& clause : output_states) {
        std::vector<bool> converted_state{belief_length, false, std::allocator<int32_t>()};

        for (const auto term : clause) {
            converted_state[std::abs(term) - 1] = (term > 0);
        }
        //Pad the state up to the length of the beliefs
        if (converted_state.size() < belief_length) {
            converted_state.insert(converted_state.end(), belief_length - converted_state.size(), false);
        }

        generated_states.emplace_back(std::move(converted_state));
    }

    return generated_states;
#endif
}

//Caluclate the hamming distance between a state and the set of beliefs
//This could probably be an std::algorithm
unsigned long state_difference(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set) noexcept {
    unsigned long min = ULONG_MAX;

    for (const auto& b : belief_set) {
        assert(state.size() == b.size());

        unsigned long count = 0;

        for (unsigned long i = 0; i < b.size(); ++i) {
            count += b[i] ^ state[i];
        }
        if (count < min) {
            min = count;
        }
    }

    return min;
}

void revise_beliefs(const std::vector<std::vector<bool>>& original_beliefs, const std::vector<std::vector<int32_t>>& formula) noexcept {
    const auto formula_states = generate_states(formula, original_beliefs.front().size());
    if (formula_states.empty()) {
        std::cerr << "Formula is unsatisfiable\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Generated state size: " << formula_states.size() << "\n";

    std::vector<std::vector<bool>> revised_beliefs;

    for (const auto& state : formula_states) {
        if (std::find(original_beliefs.cbegin(), original_beliefs.cend(), state) != original_beliefs.cend()) {
            revised_beliefs.push_back(state);
        }
    }
    if (revised_beliefs.empty()) {
        //Calculate distances and add stuff that way
        std::multimap<unsigned long, std::vector<bool>> distance_map;
        for (const auto& state : formula_states) {
            distance_map.emplace(state_difference(state, original_beliefs), state);
        }

        //Since no element is contained inside the original beliefs, no distance will be zero
        const auto min_dist = distance_map.upper_bound(0)->first;

        //Add all the beliefs that have the minimal distance from the original ones
        const auto range = distance_map.equal_range(min_dist);
        for (auto it = range.first; it != range.second; ++it) {
            revised_beliefs.push_back(it->second);
        }
    }

    assert(!revised_beliefs.empty());

    //We're done
    std::cout << "Revised belief set:\n";
    for (const auto& belief : revised_beliefs) {
        for (const auto b : belief) {
            std::cout << b;
        }
        std::cout << "\n";
    }
}
