#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>

using namespace libflo;

const std::string class_name(std::shared_ptr<flo<node, operation<node> > > &flof)
{
    for (const auto& node: flof->nodes()) {
        if (strstr(node->name().c_str(), ":") == NULL)
            continue;

        char buffer[LINE_MAX];
        strncpy(buffer, node->name().c_str(), LINE_MAX);
        strstr(buffer, ":")[0] = '\0';
        return buffer;
    }

    fprintf(stderr, "Unable to obtain class name\n");
    abort();
    return "";
}

const std::string normalize_name(std::string name)
{
    std::string norm_name = "";
    size_t last_index = 0;

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
