#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <variant>
#include <unistd.h>
#include <getopt.h>
#include "file.h"
#include "belief.h"

static struct option long_options[] = {
    {"belief_set",  required_argument, 0, 'b'},
    {"formula",     required_argument, 0, 'f'},
    {"interactive", no_argument,       0, 'i'},
    {0,         0,                 0, 0}
};

#define print_help() \
    do { \
        printf("usage options:\n"\
                "\t [i]nteractive           - Run the application in interactive mode\n"\
                "\t [b]elief_set            - The file path of the initial belief set\n"\
                "\t [f]ormula               - The file path of the revision formula\n"\
                "\t [h]elp                  - this message\n"\
                "If interactive mode is not specified, the belief_set and formula paths must be provided\n"\
                );\
    } while(0)

int main(int argc, char **argv) {
    const char *belief_path = nullptr;
    const char *formula_path = nullptr;
    bool is_interactive = false;
    for (;;) {
        int c;
        int option_index = 0;
        if ((c = getopt_long(argc, argv, "b:f:ih", long_options, &option_index)) == -1) {
            break;
        }
        switch (c) {
            case 'i':
                is_interactive = true;
                break;
            case 'b':
                belief_path = optarg;
                break;
            case 'f':
                formula_path = optarg;
                break;
            case 'h':
                [[fallthrough]];
            case '?':
                [[fallthrough]];
            default:
                print_help();
                return EXIT_SUCCESS;
        }
    }
    //Not currently supporting interactive mode yet
    if (is_interactive) {
        std::cout << "Entering interactive mode\n";
        return EXIT_SUCCESS;
    }
    if (!belief_path || !formula_path) {
        std::cerr << "Required file inputs were not provided\n";
        print_help();
        return EXIT_FAILURE;
    }

    auto [belief_format, beliefs] = read_file(belief_path);
    auto [formula_format, formula] = read_file(formula_path);

    if ((belief_format != type_format::RAW && std::get<std::vector<std::vector<int32_t>>>(beliefs).empty())
            || (belief_format == type_format::RAW && std::get<std::vector<std::vector<bool>>>(beliefs).empty())) {
        std::cerr << "Error parsing belief file\n";
        return EXIT_FAILURE;
    }
    if ((formula_format != type_format::RAW && std::get<std::vector<std::vector<int32_t>>>(formula).empty())
            || (formula_format == type_format::RAW && std::get<std::vector<std::vector<bool>>>(formula).empty())) {
        std::cerr << "Error parsing formula file\n";
        return EXIT_FAILURE;
    }

    if (belief_format != type_format::RAW) {
        if (belief_format == type_format::CNF) {
            //Convert CNF to DNF
            beliefs = convert_normal_forms(std::get<std::vector<std::vector<int32_t>>>(beliefs));
        }
        //Convert DNF to raw
        beliefs = convert_dnf_to_raw(std::get<std::vector<std::vector<int32_t>>>(beliefs));
    }
    if (formula_format != type_format::CNF) {
        if (belief_format == type_format::RAW) {
            //Get DNF from raw data
            formula = convert_raw(std::get<std::vector<std::vector<bool>>>(formula));
        }
        //Convert that DNF into CNF
        formula = convert_normal_forms(std::get<std::vector<std::vector<int32_t>>>(formula));
    }

    std::cout << "Initial belief states:\n";
    for (const auto& state : std::get<std::vector<std::vector<bool>>>(beliefs)) {
        for (unsigned long i = 0; i < state.size(); ++i) {
            std::cout << state[i];
        }
        std::cout << "\n";
    }

    std::cout << "Revision formula:\n";
    for (const auto& clause : std::get<std::vector<std::vector<int32_t>>>(formula)) {
        for (const auto term : clause) {
            std::cout << term << " ";
        }
        std::cout << "\n";
    }

    revise_beliefs(std::get<std::vector<std::vector<bool>>>(beliefs), std::get<std::vector<std::vector<int32_t>>>(formula));

    return EXIT_SUCCESS;
}
