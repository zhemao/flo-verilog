#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>
#include <libflo/version.h++>
#include <libflo/sizet_printf.h++>
#include <string.h>

#include "version.h"
#include "libflo2v/generation.hpp"

#include <iostream>

using namespace libflo;

int main(int argc, const char **argv)
{
    /* Prints a simple help text if there isn't a sane set of
     * command-line arguments, which in our case means just a single
     * argument. */
    if (argc <= 1 || argc > 2) {
        std::cerr << argv[0] << " <flo>: generate verilog from a flo file\n";
        exit(EXIT_FAILURE);
    }

    /* If "--version" was passed then print out the version of this
     * program along with the version of libflo that this was linked
     * against. */
    if (strcmp(argv[1], "--version") == 0) {
        std::cout << argv[0] << " " << PCONFIGURE_VERSION
                  << " (using libflo " << libflo::version() << ")\n";
        exit(0);
    }

    auto flof = flo<node, operation<node> >::parse(argv[1]);

    gen_flo(flof);
}
