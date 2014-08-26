#ifndef LIBSTEP_STEP_H
#define LIBSTEP_STEP_H

#include "libstep/action.hpp"

#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace libstep {
    typedef std::shared_ptr<action> action_ptr;
    class step {
        protected:
            std::vector<action_ptr> _actions;
        public:
            std::vector<action_ptr>& actions(void) { return _actions; }
            void add_action(action_ptr act)
            {
                _actions.push_back(act);
            }
            static const std::shared_ptr<step> parse(
                    const std::string filename);
            void dump(std::ostream &stream);
    };
}

#endif
