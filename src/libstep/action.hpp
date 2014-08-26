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
            const long _value;
        public:
            action(const action_type cAt,
                   const std::string cModule,
                   const std::string cSignal,
                   const long cValue) :
                _at(cAt),
                _module(cModule),
                _signal(cSignal),
                _value(cValue) {}
            action_type at(void) { return _at; }
            std::string module(void) { return _module; }
            std::string signal(void) { return _signal; }
            long value(void) { return _value; }
            std::string to_string(void)
            {
                switch (_at) {
                case action_type::STEP:
                    return "step " + std::to_string(_value);
                case action_type::RESET:
                    return "reset " + std::to_string(_value);
                case action_type::WIRE_POKE:
                    return "wire_poke " + _module + "." + _signal
                            + " " + std::to_string(_value);
                case action_type::QUIT:
                    return "quit";
                default:
                    return "";
                }
            }
    };
}

#endif
