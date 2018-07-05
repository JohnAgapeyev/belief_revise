#ifndef BELIEF_H
#define BELIEF_H

#include <vector>
#include <functional>
#include <bitset>
#include <unordered_map>

template<typename T, typename... U>
constexpr auto get_function_address(const std::function<T(U...)>& f) {
    return *f.template target<T(*)(U...)>();
}

template<typename T, typename... U>
constexpr auto operator==(const std::function<T(U...)>& lhs, const std::function<T(U...)>& rhs) {
    return get_function_address(lhs) == get_function_address(rhs);
}

extern std::function<unsigned long(const std::vector<bool>&, const std::vector<std::vector<bool>>&)> total_preorder;

//Generates a vector of all possible states given a formula clause list and the total belief length
std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept;

//Calculates the difference between a state and the existing beliefs
//Currently uses Hamming weight
unsigned long state_difference(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set);
unsigned long hamming(const std::bitset<512>& state, const std::vector<std::bitset<512>>& belief_set) noexcept;

unsigned long pd_hamming(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set, const std::unordered_map<int32_t, unsigned long>& orderings);
unsigned long pd_hamming_bitset(const std::bitset<512>& state, const std::vector<std::bitset<512>>& belief_set, const std::unordered_map<int32_t, unsigned long>& orderings) noexcept;

//The main revision function
void revise_beliefs(std::vector<std::vector<bool>>& original_beliefs, const std::vector<std::vector<int32_t>>& formula, const std::unordered_map<int32_t, unsigned long> orderings = {}, const char *output_file = nullptr) noexcept;

//Minimize the provided formula using tabular reduction
std::vector<std::vector<int32_t>> minimize_output(const std::vector<std::vector<int32_t>>& original_terms) noexcept;

#endif
