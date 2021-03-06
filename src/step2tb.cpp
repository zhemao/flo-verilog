#include <libflo/flo.h++>

#include <iostream>
#include <fstream>

#include "libflo2v/generation.hpp"
#include "libstep/step.hpp"

using namespace libflo;

#ifndef CLOCK_PERIOD
#define CLOCK_PERIOD 2
#endif

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <step> <flo>\n";
        return -1;
    }

    std::string flopath(argv[2]);
    auto dotpos = flopath.rfind(".flo");
    if (dotpos == std::string::npos) {
        fprintf(stderr, "Not a flo file\n");
        exit(EXIT_FAILURE);
    }
    auto outpath = flopath.substr(0, dotpos) + "_tb.v";
    std::ofstream output(outpath);

    auto stepf = libstep::step::parse(argv[1]);
    auto flof = flo<node, operation<node> >::parse(argv[2]);

    flo2v::gen_step(flof, stepf, CLOCK_PERIOD, output);

    return 0;
}
