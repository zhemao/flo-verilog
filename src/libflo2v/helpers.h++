#ifndef FLO2V_HELPERS_H
#define FLO2V_HELPERS_H

#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>

using namespace libflo;

typedef std::shared_ptr<node> nodeptr;
typedef std::shared_ptr<operation<node> > opptr;

// find the name of the top-level module for this flo file
const std::string class_name(std::shared_ptr<flo<node, operation<node> > > &flof)
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
    std::string norm_name = "";
    size_t last_index = 0;

    // Constants with known widths should have the width specified
    if (node->known_width() && node->is_const()) {
        return std::to_string(node->width()) + "'d" + node->name();
    }

    // If there are any colons, replace them with double underscores
    // This is rather arbitrary, but it produces a valid Verilog
    // identifier that the user is unlikely to specify themselves
    while (true) {
        size_t index = name.find(":", last_index);
        if (index == std::string::npos) {
            norm_name += name.substr(last_index);
            break;
        }
        norm_name += name.substr(last_index, index - last_index) + "__";
        last_index = index + 1;
    }
    return norm_name;
}

#endif
