#ifndef BELIEF_H
#define BELIEF_H

#include <vector>

std::vector<std::vector<bool>> generate_states(const std::vector<std::vector<int32_t>>& clause_list, const unsigned long belief_length) noexcept;

unsigned long state_difference(const std::vector<bool>& state, const std::vector<std::vector<bool>>& belief_set) noexcept;

void revise_beliefs(const std::vector<std::vector<bool>>& original_beliefs, const std::vector<std::vector<int32_t>>& formula) noexcept;

#endif
