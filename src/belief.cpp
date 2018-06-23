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
#include <functional>
#include <unordered_map>
#include <omp.h>
#include "belief.h"
#include "utils.h"
#include "interactive.h"

/*
 * This is an example preordering that one can write, if they desire something other than the hamming distance.
 * If you desire a specialization of this function for performance, see the README for details.
 */
unsigned long example_preorder(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set) {
    if (state.size() < 3) {
        return 2;
    }
    if (state[0]) {
        return 0;
    } else if (state[1]) {
        return 1;
    } else if (state[2]) {
        return 4;
    } else {
        return 7;
    }
}

//THIS IS WHERE THE PRE-ORDER IS ASSIGNED, CHANGE THIS IF YOU WANT A DIFFERENT PRE-ORDER
std::function<unsigned long(const std::vector<bool>&, const std::vector<std::vector<bool>>&)> total_preorder = state_difference;

//Helper function that determines if a given state satisfies the formula
static bool satisfies(const std::vector<bool>& state, const std::vector<std::vector<int32_t>>& clause_list) noexcept {
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

//Helper function that determines if a given state satisfies the formula
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

//Generates all possible states given a clause list and the final belief length
//This grabs the results output from the All-SAT solver, and brute-force pads each output up to belief_length bits
std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept {
    for (const auto& clause : clause_list) {
        for (const auto term : clause) {
            if (std::abs(term) > belief_length) {
                std::cerr << "Invalid input data\n";
                std::cerr << "Formula contains more variables than the belief set\n";
                exit(EXIT_FAILURE);
            }
        }
    }

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
        std::vector<bool> converted_state{belief_length, false, std::allocator<bool>()};

        for (const auto term : clause) {
            converted_state[std::abs(term) - 1] = (term > 0);
        }
        //Pad the state up to the length of the beliefs
#pragma omp parallel for shared(generated_states, clause_list) firstprivate(converted_state) schedule(static)
        for (uint64_t mask = 0; mask < (1ull << (belief_length - std::abs(clause.back()))); ++mask) {
            std::bitset<64> bs{mask};

            //Add the bits to the end of the state
            for (unsigned long i = std::abs(clause.back()); i < belief_length; ++i) {
                converted_state[i] = bs[i - std::abs(clause.back())];
            }

            if (!satisfies(converted_state, clause_list)) {
                continue;
            }

#pragma omp critical
            generated_states.emplace_back(converted_state);
        }
    }

    return generated_states;
}

//Caluclate the hamming distance between a state and the set of beliefs
unsigned long state_difference(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set) {
    unsigned long min_dist = ULONG_MAX;

#pragma omp parallel for reduction(min: min_dist) schedule(static)
    for (auto it = belief_set.cbegin(); it < belief_set.cend(); ++it) {
        assert(state.size() == it->size());

        unsigned long count = 0;
#pragma omp simd reduction(+: count)
        for (unsigned long i = 0; i < it->size(); ++i) {
            count += (*it)[i] ^ state[i];
        }
        min_dist = std::min(min_dist, count);
    }

    return min_dist;
}

//Same as the other hamming distance formula, but uses a bitset for MUCH faster evaluation
//Must be different name since overloading functions messes with function comparisons in order to actually call this function
unsigned long hamming(const std::bitset<512>& state, const std::vector<std::bitset<512>>& belief_set) noexcept {
    unsigned long min_dist = ULONG_MAX;

#pragma omp parallel for reduction(min: min_dist) schedule(static)
    for (auto it = belief_set.cbegin(); it < belief_set.cend(); ++it) {
        assert(state.size() == it->size());
        min_dist = std::min(min_dist, (state ^ *it).count());
    }

    return min_dist;
}

unsigned long pd_hamming(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set, const std::unordered_map<int32_t, unsigned long>& orderings) {
    unsigned long min_dist = ULONG_MAX;

#pragma omp parallel for reduction(min: min_dist) schedule(static)
    for (auto it = belief_set.cbegin(); it < belief_set.cend(); ++it) {
        assert(state.size() == it->size());

        unsigned long count = 0;
#pragma omp simd reduction(+: count)
        for (unsigned long i = 0; i < it->size(); ++i) {
            count += ((*it)[i] ^ state[i]) * ((i + 1 > orderings.size()) ? 1 : orderings.find(i + 1)->second);
        }
        min_dist = std::min(min_dist, count);
    }

    return min_dist;
}

unsigned long pd_hamming_bitset(const std::bitset<512>& state, const std::vector<std::bitset<512>>& belief_set, const std::unordered_map<int32_t, unsigned long>& orderings) noexcept {
    unsigned long min_dist = ULONG_MAX;

#pragma omp parallel for reduction(min: min_dist) schedule(static)
    for (auto it = belief_set.cbegin(); it < belief_set.cend(); ++it) {
        assert(state.size() == it->size());

        unsigned long count = 0;
        const auto result = (state ^ *it);

        for (auto i = 0; i < 512; ++i) {
            count += result[i] * ((i + 1lu > orderings.size()) ? 1 : orderings.find(i + 1)->second);
        }

        min_dist = std::min(min_dist, count);
    }

    return min_dist;
}

//The main revision function
//Original beliefs must contain equal length bit assignments representing the state of each variable
//The formula must be in CNF format
void revise_beliefs(std::vector<std::vector<bool>>& original_beliefs, const std::vector<std::vector<int32_t>>& formula, const std::unordered_map<int32_t, unsigned long> orderings) noexcept {
    auto formula_states = generate_states(formula, original_beliefs.front().size());
    if (formula_states.empty()) {
        std::cerr << "Formula is unsatisfiable\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Generated state size: " << formula_states.size() << "\n";

    std::vector<std::vector<bool>> revised_beliefs;

    if (!std::is_sorted(formula_states.begin(), formula_states.end())) {
        std::sort(formula_states.begin(), formula_states.end());
    }
    if (!std::is_sorted(original_beliefs.begin(), original_beliefs.end())) {
        std::sort(original_beliefs.begin(), original_beliefs.end());
    }

    std::cout << "Done sorting\n";

    std::set_intersection(formula_states.cbegin(), formula_states.cend(), original_beliefs.cbegin(), original_beliefs.cend(), std::back_inserter(revised_beliefs));

    std::cout << "Done intersection\n";

    if (revised_beliefs.empty()) {
        //Calculate distances and add stuff that way
        std::multimap<unsigned long, std::vector<bool>> distance_map;

        //Specialization of hamming distance to efficiently use bitsets
        if (total_preorder == decltype(total_preorder)(state_difference) && formula_states.front().size() <= 512) {
            //512 bits because that is infeasible to compute
            //Could do 256, but theoretically, I might be able to do that
            std::vector<std::bitset<512>> formula_bits{formula_states.size(), {}, std::allocator<std::bitset<512>>()};
            std::vector<std::bitset<512>> belief_bits;

            belief_bits.reserve(original_beliefs.size());

#pragma omp parallel shared(formula_bits, belief_bits)
            {
#pragma omp for schedule(static) nowait
                for (auto it = formula_states.cbegin(); it < formula_states.cend(); ++it) {
                    std::bitset<512> bs;
                    for (unsigned int i = 0; i < 512; ++i) {
                        bs[i] = (i < it->size()) ? (*it)[i] : false;
                    }
#pragma omp critical(form)
                    formula_bits[formula_bits.size() - std::distance(it, formula_states.cend())] = std::move(bs);
                }
#pragma omp for schedule(static) nowait
                for (auto it = original_beliefs.cbegin(); it < original_beliefs.cend(); ++it) {
                    std::bitset<512> bs;
                    for (unsigned int i = 0; i < 512; ++i) {
                        bs[i] = (i < it->size()) ? (*it)[i] : false;
                    }
#pragma omp critical(bell)
                    belief_bits.emplace_back(std::move(bs));
                }
            }

            std::cout << "Done conversion\n";

            for (unsigned int i = 0; i < formula_states.size(); ++i) {
                if (orderings.empty()) {
                    distance_map.emplace(hamming(formula_bits[i], belief_bits), formula_states[i]);
                } else {
                    distance_map.emplace(pd_hamming_bitset(formula_bits[i], belief_bits, orderings), formula_states[i]);
                }
            }
        } else {
            for (const auto& state : formula_states) {
                if (orderings.empty()) {
                    distance_map.emplace(total_preorder(state, original_beliefs), state);
                } else {
                    distance_map.emplace(pd_hamming(state, original_beliefs, orderings), state);
                }
            }
        }

        //Grab every element whose key is equal to the lowest key in the map
        const auto min_dist = distance_map.lower_bound(0)->first;

        std::cout << min_dist << "\n";

        //Add all the beliefs that have the minimal distance from the original ones
        const auto range = distance_map.equal_range(min_dist);
        for (auto it = range.first; it != range.second; ++it) {
            revised_beliefs.push_back(it->second);
        }
    }

    assert(!revised_beliefs.empty());

    //We're done
    std::cout << "Revised belief set:\n";

    if (verbose) {
        for (const auto& belief : revised_beliefs) {
            for (unsigned long i = 0; i < belief.size(); ++i) {
                int32_t term = i + 1;
                if (!belief[i]) {
                    term *= -1;
                }
                std::cout << term << " ";
            }
            std::cout << "\n";
        }
    }

    print_formula_dnf(convert_to_num(revised_beliefs));

    for (const auto& first : revised_beliefs) {
        std::vector<int32_t> converted_term;
        for (const auto& second : revised_beliefs) {
            if (first == second) {
                continue;
            }
            unsigned long count = 0;
            for (unsigned long i = 0; i < first.size(); ++i) {
                count += first[i] ^ second[i];
            }
            if (count == 1) {
                goto minimize;
            }
        }
    }
    return;

minimize:
    std::cout << "Minimization is possible\n";

    std::cout << "Initial pre-minized state size: " << revised_beliefs.size() << "\n";

    auto minimized = minimize_output(convert_to_num(revised_beliefs));
    for (;;) {
        unsigned long old_size = minimized.size();

        std::cout << "Minimized Size: " << old_size << "\n";
        unsigned long long old_sum = 0;
        for (const auto& clause : minimized) {
            old_sum += clause.size();
        }
        std::cout << "Average clause size: " << (old_sum / old_size) << "\n";

        minimized = minimize_output(minimized);

        unsigned long long new_sum = 0;
        for (const auto& clause : minimized) {
            new_sum += clause.size();
        }

        if (old_size == minimized.size() && new_sum == old_sum) {
            minimized = minimize_output(minimized);
            //Print minimized
            std::cout << "Minimized states:\n";
            if (verbose) {
                for (const auto& belief : minimized) {
                    for (const auto term : belief) {
                        std::cout << term << " ";
                    }
                    std::cout << "\n";
                }
            }
            print_formula_dnf(minimized);
            break;
        }
        for (auto& clause : minimized) {
            clause.shrink_to_fit();
        }
        minimized.shrink_to_fit();
    }
}

std::vector<std::vector<int32_t>> minimize_output(const std::vector<std::vector<int32_t>>& original_terms) noexcept {
    std::vector<std::vector<int32_t>> output;
    output.reserve(original_terms.size());

    std::vector<int32_t> converted_term;
    converted_term.reserve(original_terms.front().size());

    //for (const auto& first : original_terms) {
#pragma omp parallel for schedule(static) shared(output) firstprivate(converted_term)
    for (auto it = original_terms.cbegin(); it < original_terms.cend(); ++it) {
        const auto& first = *it;
        bool term_minimized = false;
        for (const auto& second : original_terms) {
            if (first == second) {
                continue;
            }
            unsigned long count = 0;
            unsigned long index = -1;
            for (unsigned long i = 0; i < std::min(first.size(), second.size()); ++i) {
                if ((first[i] * -1) == second[i]) {
                    ++count;
                    index = i;
                }
            }
            if (count != 1) {
                continue;
            }
            //Count is 1, minimize
            for (unsigned long i = 0; i < first.size(); ++i) {
                if (i == index) {
                    continue;
                }
                converted_term.emplace_back(first[i]);
            }
#pragma omp critical
            output.emplace_back(converted_term);
            converted_term.clear();
            term_minimized = true;
        }
        if (!term_minimized) {
            for (unsigned long i = 0; i < first.size(); ++i) {
                converted_term.emplace_back(first[i]);
            }
        }
#pragma omp critical
        output.emplace_back(converted_term);
        converted_term.clear();
    }


    for (auto& clause : output) {
        std::sort(clause.begin(), clause.end());
    }

    std::sort(output.begin(), output.end());
    output.erase(std::unique(output.begin(), output.end()), output.end());
    output.erase(std::remove_if(output.begin(), output.end(), [](const auto& clause){return clause.empty();}), output.end());

    //Remove supersets of existing clauses
    //This results in smaller end results, but doesn't really affect the initial state increase
    output.erase(std::remove_if(output.begin(), output.end(),
                [&] (const auto& clause) {
                    for (const auto& other : output) {
                        if (clause == other || other.empty()) {
                            continue;
                        }
                        if (std::includes(clause.cbegin(), clause.cend(), other.cbegin(), other.cend())) {
                            return true;
                        }
                    }
                    return false;
                }
                ), output.end());

    const auto abs_cmp = [](const auto& lhs, const auto& rhs){return std::abs(lhs) < std::abs(rhs);};
    for (auto& clause : output) {
        std::sort(clause.begin(), clause.end(), abs_cmp);
    }

    return output;
}
