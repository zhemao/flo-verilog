#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>
#include <libflo/version.h++>
#include <libflo/sizet_printf.h++>
#include <getopt.h>

#include "version.h"
#include "libflo2v/generation.hpp"

#include <iostream>
#include <string>
#include <fstream>

using namespace libflo;

static void print_help(const char *prog_name)
{
    std::cerr << prog_name << " (--version | <flo>):"
              << " generate verilog from a flo file\n";
}

int main(int argc, char *argv[])
{
    const struct option long_options[] = {
        {"version", 0, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    int opt;
    bool version = false;

    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) > 0) {
        switch (opt) {
        case 'v':
            version = true;
            break;
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* If "--version" was passed then print out the version of this
     * program along with the version of libflo that this was linked
     * against. */
    if (version) {
        std::cout << argv[0] << " " << PCONFIGURE_VERSION
                  << " (using libflo " << libflo::version() << ")\n";
        exit(0);
    }

    if (optind >= argc) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    std::string outpath(argv[optind]);
    auto dotpos = outpath.rfind(".flo");

    if (dotpos == std::string::npos) {
        std::cerr << "Input is not a flo file\n";
        exit(EXIT_FAILURE);
    }

    outpath.replace(dotpos, 4, ".v");

    auto flof = flo<node, operation<node> >::parse(argv[optind]);
    std::ofstream vstream(outpath.c_str());

    flo2v::gen_flo(flof, vstream);
}
