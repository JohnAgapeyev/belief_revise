# belief_revise
This application computes the [Belief Revision](https://en.wikipedia.org/wiki/Belief_revision) operator on a set of initial beliefs, revised based on a given formula.

## Usage:
Interactive mode:
`./bin/belief_rev -i`

File input:
`./bin/belief_rev -b belief_data.txt -f formula_data.txt`

Usage help:
`./bin/belief_rev -h`

## Input formats:
Input data can be entered in one of 3 formats:
 - CNF, aka Conjunctive Normal Form
 - DNF, aka Disjunctive Normal Form
 - Raw Hex, where the bits of the hex values are the assignments of each variable at their respective indices

All input files MUST contain a "problem line" that details the input type used for that file
A problem line is defined as a line starting with the letter 'p', followed by either 'cnf', 'dnf', or 'raw'
The file parsing is case sensitive
The problem line MUST be the first non-comment line in the file

Comments can be added by making the first character of the line 'c'

CNF and DNF formatted data follow the [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html) file format
Raw hex requires the entire line contain a valid hex string

## Compilation:
To compile simply run the following commands:
```
cmake .
make
```

Compilation requires:
 - CMake 3.5.1 or later
 - POSIX Operating System
 - OpenMP 4.0 or later compliant compiler
 - C++17 compliant compiler

## Custom Pre-orders
This application does support arbitrary preorders, but due to the potential complexity of a preorder, there is no runtime interface for entering one.
Instead, one must modify the source code in order to provide the appropriate function.

The location of the required modifications is src/belief.cpp, and is at the top of the file, right after the header includes.

All preorders must follow the following prototype:
`unsigned long x(const std::vector<bool>&, const std::vector<std::vector<bool>>&)`

This new pre-order function will be called, once the `total_preorder` function object is assigned.
There is an example pre-order function provided that shows how individual bits may be referenced.

To assign your new function as the new pre-order, find and modify the following line in src/belief.cpp
```
const std::function<unsigned long(const std::vector<bool>&, const std::vector<std::vector<bool>>&)> total_preorder = state_difference;
```

This will ensure your new function is selected at runtime, as opposed to the default.

### Custom Pre-order Specialization
If one desires to specialize the preorder function for performance reasons, there are some additional steps that must be followed.

In the `revise_beliefs` function in src/belief.cpp, there is a section of code that looks somewhat like this:
```
    if (total_preorder == decltype(total_preorder)(state_difference)) {
        //A bunch of code that converts the std::vector<bool> containers to std::bitset<512> for SIMD-optimized Hamming distance
        ...
        for (unsigned int i = 0; i < formula_states.size(); ++i) {
            distance_map.emplace(hamming(formula_bits[i], belief_bits), formula_states[i]);
        }
    } else {
        for (const auto& state: formula_states) {
            distance_map.emplace(total_preorder(state, original_beliefs), state);
        }
    }
```

In order to add a specialization, there are 3 changes one must make.
 - Add an else-if clause that compares the preorder function with your desired function override.
 - Call your function
 - Add the result into `distance_map`, ensuring the unsigned long result, and the original std::vector<bool> state are both entered correctly as part of the same entry.

This will ensure that your function can be specialized in its implementation, if a different data format is required, and it's cheaper to convert before the function, rather than inside it.

