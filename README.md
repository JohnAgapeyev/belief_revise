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

