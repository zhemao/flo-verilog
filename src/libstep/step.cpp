#include "libstep/step.hpp"
#include "libstep/exceptions.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

namespace libstep {
    static std::vector<std::string> split(
            const std::string &s, char delim)
    {
        std::stringstream stream(s);
        std::string item;
        std::vector<std::string> elems;

        while (std::getline(stream, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    static void split_signal_name(std::string &fullname,
                                         std::string &module,
                                         std::string &signal)
    {
        auto parts = split(fullname, '.');
        if (parts.size() != 2)
            throw malformed_exception();
        module = parts[0];
        signal = parts[1];
    }

    static action_ptr parse_line(std::string &line)
    {
            auto parts = split(line, ' ');
            if (parts[0] == "step") {
                if (parts.size() != 2)
                    throw malformed_exception();
                return std::shared_ptr<action>(
                        new action(action_type::STEP, "", "",
                                   std::stoi(parts[1])));
            }
            if (parts[0] == "wire_poke") {
                if (parts.size() != 3)
                    throw malformed_exception();
                std::string module, signal;
                split_signal_name(parts[1], module, signal);
                return std::shared_ptr<action>(
                        new action(action_type::WIRE_POKE,
                            module, signal,
                            std::stoi(parts[2])));
            }
            if (parts[0] == "reset") {
                if (parts.size() != 2)
                    throw malformed_exception();
                return std::shared_ptr<action>(
                        new action(action_type::RESET, "", "",
                            std::stoi(parts[1])));
            }
            if (parts[0] == "quit")
                return std::shared_ptr<action>(
                        new action(action_type::QUIT, "", "", 0));
            throw malformed_exception();
    }

    const std::shared_ptr<step> step::parse(
            const std::string filename)
    {
        std::shared_ptr<step> stepf(new step());
        std::ifstream file;

        file.open(filename);

        if (file.bad())
            throw nofile_exception();

        while (file.good()) {
            std::string line;
            std::getline(file, line);
            if (line == "")
                continue;
            auto act = parse_line(line);
            stepf->add_action(act);
        }

        file.close();

        return stepf;
    }

    void step::dump(std::ostream &stream)
    {
        for (const auto &act : _actions) {
            stream << act->to_string() << "\n";
        }
    }
}
