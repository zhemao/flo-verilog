#ifndef FLO2V_GENERATION_H
#define FLO2V_GENERATION_H

#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>

using namespace libflo;

void gen_flo(std::shared_ptr<flo<node, operation<node> > > flof);

#endif
