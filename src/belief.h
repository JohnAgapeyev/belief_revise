#ifndef BELIEF_H
#define BELIEF_H

#include <vector>

//Generates a vector of all possible states given a formula clause list and the total belief length
std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept;

//Calculates the difference between a state and the existing beliefs
//Currently uses Hamming weight
unsigned long state_difference(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set) noexcept;

//The main revision function
void revise_beliefs(std::vector<std::vector<bool>>& original_beliefs, const std::vector<std::vector<int32_t>>& formula) noexcept;

#endif
