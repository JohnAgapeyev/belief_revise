#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <variant>
#include <unistd.h>
#include <getopt.h>
#include "file.h"
#include "belief.h"
#include "interactive.h"

static struct option long_options[] = {
    {"belief_set",  required_argument, 0, 'b'},
    {"formula",     required_argument, 0, 'f'},
    {"interactive", no_argument,       0, 'i'},
    {"pd-ordering", required_argument, 0, 'p'},
    {"dalal",       no_argument,       0, 'd'},
    {"verbose",     no_argument,       0, 'v'},
    {"output",      required_argument, 0, 'o'},
    {0,         0,                 0, 0}
};

#define print_help() \
    do { \
        printf("usage options:\n"\
                "\t [i]nteractive           - Run the application in interactive mode\n"\
                "\t [b]elief_set            - The file path of the initial belief set\n"\
                "\t [f]ormula               - The file path of the revision formula\n"\
                "\t [p]d-ordering           - The file path of the pd orderings\n"\
                "\t [d]alal                 - Use the Dalal pre-order (Hamming distance)\n"\
                "\t [v]erbose               - Output in verbose mode\n"\
                "\t [o]utput                - File to output revised beliefse to\n"\
                "\t [h]elp                  - this message\n"\
                "If interactive mode is not specified, the belief_set and formula paths must be provided\n"\
                );\
    } while(0)

int main(int argc, char **argv) {
    const char *belief_path = nullptr;
    const char *formula_path = nullptr;
    const char *pd_path = nullptr;
    const char *output_file = nullptr;
    bool is_interactive = false;
    bool use_pd_ordering = false;
    for (;;) {
        int c;
        int option_index = 0;
        if ((c = getopt_long(argc, argv, "b:f:ihp:dvo:", long_options, &option_index)) == -1) {
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
            case 'd':
                use_pd_ordering = false;
                break;
            case 'p':
                use_pd_ordering = true;
                pd_path = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'o':
                output_file = optarg;
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
        auto [beliefs, formula] = run_interactive_mode();

        if (verbose) {
            std::cout << "Initial belief states:\n";
            for (const auto& state : beliefs) {
                for (unsigned long i = 0; i < state.size(); ++i) {
                    std::cout << state[i];
                }
                std::cout << "\n";
            }

            std::cout << "Revision formula:\n";
            for (const auto& clause : formula) {
                for (const auto term : clause) {
                    std::cout << term << " ";
                }
                std::cout << "\n";
            }
        }
        if (use_pd_ordering) {
            if (pd_path == nullptr) {
                std::cerr << "PD path was null when it shouldn't be\n";
                return EXIT_FAILURE;
            }
            const auto orderings = read_pd_ordering(pd_path);
            if (orderings.empty()) {
                std::cerr << "Error reading pd ordering file\n";
                return EXIT_FAILURE;
            }

            int32_t max_variable = -1;
            for (const auto& clause : beliefs) {
                for (const auto term : clause) {
                    max_variable = std::max(max_variable, std::abs(term));
                }
            }

            if (!orderings.count(max_variable)) {
                std::cerr << "PD orderings must contain assignments for all input variables\n";
                return EXIT_FAILURE;
            }
            if (verbose) {
                std::cout << "Variable orderings:\n";
                for (const auto& p : orderings) {
                    std::cout << p.first << " " << p.second << "\n";
                }
            }
            revise_beliefs(beliefs, formula, orderings, output_file);
        } else {
            revise_beliefs(beliefs, formula, {}, output_file);
        }

        return EXIT_SUCCESS;
    }
    if (!belief_path || !formula_path) {
        std::cerr << "Required file inputs were not provided\n";
        print_help();
        return EXIT_FAILURE;
    }

    auto [belief_format, beliefs] = read_file(belief_path);

    if ((belief_format != type_format::RAW && !std::get_if<std::vector<std::vector<int32_t>>>(&beliefs))
            || (belief_format == type_format::RAW && !std::get_if<std::vector<std::vector<bool>>>(&beliefs))) {
        std::cerr << "Error parsing belief file\n";
        return EXIT_FAILURE;
    } else if ((belief_format != type_format::RAW && std::get<std::vector<std::vector<int32_t>>>(beliefs).empty())
            || (belief_format == type_format::RAW && std::get<std::vector<std::vector<bool>>>(beliefs).empty())) {
        std::cerr << "Error parsing belief file\n";
        return EXIT_FAILURE;
    }

    auto [formula_format, formula] = read_file(formula_path);

    if ((formula_format != type_format::RAW && !std::get_if<std::vector<std::vector<int32_t>>>(&formula))
            || (formula_format == type_format::RAW && !std::get_if<std::vector<std::vector<bool>>>(&formula))) {
        std::cerr << "Error parsing belief file\n";
        return EXIT_FAILURE;
    } else if ((formula_format != type_format::RAW && std::get<std::vector<std::vector<int32_t>>>(formula).empty())
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
    if (verbose) {
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
    }
    if (use_pd_ordering) {
        if (pd_path == nullptr) {
            std::cerr << "PD path was null when it shouldn't be\n";
            return EXIT_FAILURE;
        }
        const auto orderings = read_pd_ordering(pd_path);
        if (orderings.empty()) {
            std::cerr << "Error reading pd ordering file\n";
            return EXIT_FAILURE;
        }

        int32_t max_variable = -1;
        for (const auto& clause : std::get<std::vector<std::vector<bool>>>(beliefs)) {
            for (const auto term : clause) {
                max_variable = std::max(max_variable, std::abs(term));
            }
        }

        if (!orderings.count(max_variable)) {
            std::cerr << "PD orderings must contain assignments for all input variables\n";
            return EXIT_FAILURE;
        }

        if (verbose) {
            std::cout << "Variable orderings:\n";
            for (const auto& p : orderings) {
                std::cout << p.first << " " << p.second << "\n";
            }
        }
        revise_beliefs(std::get<std::vector<std::vector<bool>>>(beliefs), std::get<std::vector<std::vector<int32_t>>>(formula), orderings, output_file);
    } else {
        revise_beliefs(std::get<std::vector<std::vector<bool>>>(beliefs), std::get<std::vector<std::vector<int32_t>>>(formula), {}, output_file);
    }

    return EXIT_SUCCESS;
}
