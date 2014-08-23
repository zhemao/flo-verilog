#include <libstep/step.hpp>
#include <libflo/flo.h++>

#include <iostream>

#include "libflo2v/generation.hpp"

using namespace libflo;

#ifndef CLOCK_PERIOD
#define CLOCK_PERIOD 20
#endif

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <flo> <step>\n";
        return -1;
    }

    auto flof = flo<node, operation<node> >::parse(argv[1]);
    auto stepf = libstep::step::parse(argv[2]);

    gen_step(flof, stepf, CLOCK_PERIOD);

    return 0;
}
