#include <libflo/flo.h++>
#include <libflo/node.h++>
#include <libflo/operation.h++>
#include <libflo/version.h++>
#include <libflo/sizet_printf.h++>
#include <string.h>
#include "version.h"
#include "helpers.h++"

using namespace libflo;

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

    auto mod_name = class_name(flof);
    if (mod_name == "") {
        fprintf(stderr, "Could not find class name");
    }
    fprintf(stdout, "module %s (\n", mod_name.c_str());

    bool first = true;

    for (const auto& op : flof->operations()) {
        std::string inout;
        switch (op->op()) {
        case opcode::IN:
            inout = "input";
            break;
        case opcode::OUT:
            inout = "output";
            break;
        default:
            continue;
        }
        auto dest = op->d();

        if (first) {
            first = false;
        } else {
            fprintf(stdout, ",\n");
        }

        fprintf(stdout, "\t%s [" SIZET_FORMAT ":0] %s",
                inout.c_str(), dest->width() - 1,
                normalize_name(dest->name()).c_str());
    }

    fprintf(stdout, "\n);\n");

    for (const auto& node : flof->nodes()) {
        if (!node->is_mem())
            continue;

	fprintf(stdout, "reg [" SIZET_FORMAT ":0] %s [" SIZET_FORMAT ":0];\n",
			node->width() - 1,
                        normalize_name(node->name()).c_str(),
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
