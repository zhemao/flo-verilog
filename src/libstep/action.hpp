#ifndef LIBSTEP_ACTION_H
#define LIBSTEP_ACTION_H

#include <string>

namespace libstep {
    enum class action_type {
        STEP,
        RESET,
        WIRE_POKE,
        QUIT
    };

    class action {
        protected:
            const action_type _at;
            const std::string _module;
            const std::string _signal;
            const std::string _value;
            const unsigned int _cycles;
        public:
            action(const action_type cAt,
                   const std::string cModule,
                   const std::string cSignal,
                   const std::string cValue,
                   const unsigned int cCycles) :
                _at(cAt),
                _module(cModule),
                _signal(cSignal),
                _value(cValue),
                _cycles(cCycles) {}
            action_type at(void) { return _at; }
            std::string module(void) { return _module; }
            std::string signal(void) { return _signal; }
            std::string value(void) { return _value; }
            unsigned int cycles(void) { return _cycles; }
            std::string to_string(void)
            {
                switch (_at) {
                case action_type::STEP:
                    return "step " + std::to_string(_cycles);
                case action_type::RESET:
                    return "reset " + std::to_string(_cycles);
                case action_type::WIRE_POKE:
                    return "wire_poke " + _module + "." + _signal
                            + " " + _value;
                case action_type::QUIT:
                    return "quit";
                default:
                    return "";
                }
            }
    };
}

#endif
