#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>
#include <libflo/version.h++>
#include <libflo/sizet_printf.h++>
#include <string.h>
#include "version.h"

using namespace libflo;

const std::string flo::class_name(flo<node, operation<node> > &flof) const
{
    for (const auto& node: flof.nodes()) {
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

int main(int argc, const char **argv)
{
    /* Prints a simple help text if there isn't a sane set of
     * command-line arguments, which in our case means just a single
     * argument. */
    if (argc <= 1 || argc > 2) {
        fprintf(stderr, "%s <flo>: generate verilog from a flo file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* If "--version" was passed then print out the version of this
     * program along with the version of libflo that this was linked
     * against. */
    if (strcmp(argv[1], "--version") == 0) {
        fprintf(stderr, "%s %s (using libflo %s)\n",
                argv[0], PCONFIGURE_VERSION, libflo::version());
        exit(0);
    }

    auto flof = flo<node, operation<node> >::parse(argv[1]);

    auto mod_name = flof.class_name();
    if (mod_name == "") {
        fprintf(stderr, "Could not find class name");
    }
    fprintf(stdout, "module %s (\n", mod_name.c_str());

    for (const auto& node : flof->nodes()) {
        if (!node->is_mem())
            continue;

	fprintf(stdout, "reg [" SIZET_FORMAT ":0] %s [" SIZET_FORMAT ":0];\n",
			node->width() - 1,
			node->name().c_str(),
			node->depth() - 1);

    }

    for (const auto& op : flof->operations()) {
        for (const auto& node: op->operands()) {
            if (node->known_width() == false) {
                fprintf(stderr, "Unknown width of node '%s' in '%s'\n",
                        node->name().c_str(),
                        op->to_string().c_str()
                    );
                abort();
            }
        }
    }

    fprintf(stdout, "endmodule\n");
}
