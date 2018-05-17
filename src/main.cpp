#include <iostream>
#include <cstdlib>
#include "file.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Needs a filename\n";
        return EXIT_FAILURE;
    }
    auto data = read_file(argv[1]);
    if (data.first.empty() || data.second.empty()) {
        std::cerr << "Error parsing input file\n";
        return EXIT_FAILURE;
    }

    std::cout << "Initial belief states:\n";
    for (const auto& state : data.first) {
        for (int i = 0; i < state.size(); ++i) {
            std::cout << state[i];
        }
        std::cout << "\n";
    }

    std::cout << "Revision formula:\n";
    for (const auto& clause : data.second) {
        for (const auto term : clause) {
            std::cout << term << " ";
        }
        std::cout << "\n";
    }

    return EXIT_SUCCESS;
}
