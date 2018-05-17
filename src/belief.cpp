#include <vector>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <climits>
#include <map>
#include <iostream>
#include <cmath>
#include <bitset>
#include "belief.h"

//This is an n bit brute force where n is the number of variables in the clause list
//I don't like this, but unless I settle for 1 state per formula, my only option is an ALL-SAT solver
//Which is pretty damn rare, and I can't find significant information outside of 1 or 2 papers
std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept {
    int32_t variable_count = 0;

    //Find maximum term
    for (const auto& clause : clause_list) {
        for (const auto term : clause) {
            if (std::abs(term) > variable_count) {
                variable_count = std::abs(term);
            }
        }
    }

    //We need to brute force variable_count numbers of variables

    if (variable_count > 64) {
        //Undefined behaviour, and frankly, we'll never brute force 64 bits
        std::cerr << "State generation out of range\n";
        abort();
    }

    std::vector<std::vector<bool>> generated_states;

    for (uint64_t mask = 0; mask < (1ull << (belief_length)); ++mask) {
        std::bitset<64> bs{mask};
        std::vector<bool> good_state;

        for (const auto& clause : clause_list) {
            for (const auto term : clause) {
                if (bs[std::abs(term) - 1] == (term < 0)) {
                    //Term is false, this is not a valid state
                    goto bad_state;
                }
            }
        }

        //Set the good state
        for (unsigned long i = 0; i < belief_length; ++i) {
            good_state.push_back(bs[i]);
        }
        generated_states.emplace_back(std::move(good_state));

bad_state:
        continue;
    }

    return generated_states;
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

    for (const auto& belief : revised_beliefs) {
        for (const auto& clause : formula) {
            for (const auto term : clause) {
                if (belief[std::abs(term) - 1] == (term < 0)) {
                    //Term is false, this is not a valid state
                    std::cerr << "Revised belief does not satisfy provided formula\n";
                    abort();
                }
            }
        }
    }

    //We're done
    std::cout << "Revised belief set:\n";
    for (const auto& belief : revised_beliefs) {
        for (const auto b : belief) {
            std::cout << b;
        }
        std::cout << "\n";
    }
}
