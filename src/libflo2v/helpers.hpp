#ifndef FLO2V_HELPERS_H
#define FLO2V_HELPERS_H

#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>

using namespace libflo;

namespace flo2v {

    typedef std::shared_ptr<node> nodeptr;
    typedef std::shared_ptr<operation<node> > opptr;

    // find the name of the top-level module for this flo file
    const std::string class_name(
            std::shared_ptr<flo<node, operation<node> > > &flof)
    {
        for (const auto& node: flof->nodes()) {
            std::string name = node->name();
            size_t index = name.find(":");
            if (index == std::string::npos)
                continue;

            return name.substr(0, index);
        }

        fprintf(stderr, "Unable to obtain class name\n");
        abort();
        return "";
    }

    /**
     * convert a flo node into an equivalent Verilog expression
     */
    const std::string node_name(nodeptr node)
    {
        std::string name = node->name();
        std::vector<std::string> sections;
        size_t last_index = 0;

        // Constants with known widths should have the width specified
        if (node->known_width() && node->is_const())
            return std::to_string(node->width()) + "'d" + node->name();

        // If there are any colons or double colons,
        // replace them with underscores
        while (true) {
            size_t index = name.find(":", last_index);
            if (index == std::string::npos) {
                sections.push_back(name.substr(last_index));
                break;
            }
            sections.push_back(name.substr(last_index,
                        index - last_index) + "_");
            if (index + 1 < name.length() && name[index + 1] == ':') {
                // if it's a double colon, skip both of them
                last_index = index + 2;
            } else {
                last_index = index + 1;
            }
        }

        // If it is a simple name with one section, just return it
        if (sections.size() == 1)
            return sections[0];

        std::vector<std::string>::iterator iter = sections.begin();
        // ignore the first section
        iter++;
        // connect the sections with underscores
        std::string norm_name = *iter;
        iter++;
        for ( ; iter != sections.end(); iter++)
            norm_name += "_" + *iter;

        return norm_name;
    }
}

#endif
