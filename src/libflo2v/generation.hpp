#ifndef FLO2V_GENERATION_H
#define FLO2V_GENERATION_H

#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>
#include <libstep/step.hpp>
#include <ostream>

using namespace libflo;

namespace flo2v {
    void gen_flo(std::shared_ptr<flo<node, operation<node> > > flof,
                 std::ostream &out);
    void gen_step(std::shared_ptr<flo<node, operation<node> > > flof,
                  std::shared_ptr<libstep::step> stepf, size_t clock_period);
}

#endif
