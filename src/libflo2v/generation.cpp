#include "generation.hpp"
#include "helpers.hpp"

#include <iostream>

using namespace libflo;

namespace flo2v {

    static void gen_bin_op(std::ostream &out,
            std::string op, nodeptr d, nodeptr s, nodeptr t)
    {
        out << "assign " << node_name(d) << " = "
            << node_name(s) << " " << op << " " << node_name(t) << ";\n";
    }

    static void gen_un_op(std::ostream &out,
            std::string op, nodeptr d, nodeptr s)
    {
        out << "assign " << node_name(d) << " = " << op
            << node_name(s) << ";\n";
    }

    static void gen_mem(std::ostream &out, nodeptr node)
    {
        out << "reg [" << (node->width() - 1) << ":0] "
            << node_name(node) << " ["
            << (node->depth() - 1) << ":0];\n";
    }

    static void gen_lshift(std::ostream &out, nodeptr d, nodeptr s, nodeptr t)
    {
        if (s->width() < d->width()) {
            size_t zero_width = d->width() - s->width();
            out << "assign " << node_name(d) << " = {"
                << zero_width << "'d0, " << node_name(s)
                << "} << " << node_name(t) << ";\n";
            return;
        }
        if (s->width() > d->width()) {
            out << "assign " << node_name(d) << " = "
                << node_name(s) << "[" << (d->width() - 1)
                << ":0] << " << node_name(t) << ";\n";
            return;
        }
        gen_bin_op(out, "<<", d, s, t);
    }

    static void gen_selection(std::ostream &out,
            nodeptr d, nodeptr s, nodeptr t)
    {
        size_t width = d->width();
        size_t start = std::stoi(t->name());
        size_t highest = start + width;

        if (highest > s->width()) {
            size_t extend = highest - s->width();
            out << "assign " << node_name(d) << " = "
                << "{" << extend << "'d0, "
                << node_name(s) << "[" << (s->width() - 1) << ":"
                << start << "]};\n";
            return;
        }

        out << "assign " << node_name(d) << " = "
            << node_name(s) << "["
            << (highest - 1) << ":" << start << "];\n";
    }

    static void gen_rshift(std::ostream &out, nodeptr d, nodeptr s, nodeptr t)
    {
        // Flo uses right shifts by constants
        // to select bits out of signals.
        // This requires special handling in Verilog
        if (t->is_const()) {
            gen_selection(out, d, s, t);
            return;
        }

        if (s->width() < d->width()) {
            size_t zero_width = d->width() - s->width();
            out << "assign " << node_name(d) << " = {"
                << zero_width << "'d0, " << node_name(s)
                << "} >> " << node_name(t) << ";\n";
            return;
        }

        gen_bin_op(out, ">>", d, s, t);
    }

    static void gen_cat(std::ostream &out, nodeptr d, nodeptr s, nodeptr t)
    {
        out << "assign " << node_name(d) << " = {"
            << node_name(s) << ", " << node_name(t) << "};\n";
    }

    static void gen_decl(std::ostream &out, std::string typ, nodeptr d)
    {
        out << typ << " [" << (d->width() - 1) << ":0] "
            << node_name(d) << ";\n";
    }

    static void gen_reg_assign(std::ostream &out, nodeptr reg, nodeptr val)
    {
        out << "\t" << node_name(reg) << " <= " << node_name(val) << ";\n";
    }

    static void gen_mux(std::ostream &out,
            nodeptr d, nodeptr s, nodeptr t, nodeptr u)
    {
        out << "assign " << node_name(d) << " = " << "(" << node_name(s)
            << ") ? " << node_name(t) << " : " << node_name(u) << ";\n";
    }

    static void gen_write(std::ostream &out,
            nodeptr en, nodeptr mem, nodeptr addr, nodeptr val)
    {
        out << "\tif (" << node_name(en) << ") "
            << node_name(mem) << "[" << node_name(addr) << "] <= "
            << node_name(val) << ";\n";
    }

    static void gen_init(std::ostream &out,
            nodeptr mem, nodeptr addr, nodeptr val)
    {
        out << "\t\t" << node_name(mem) << "["
            // don't use node_name for addr, otherwise it will
            // try to put the wrong width on it
            << addr->name() << "] <= " << node_name(val) << ";\n";
    }

    static void gen_read(std::ostream &out,
            nodeptr d, nodeptr mem, nodeptr addr)
    {
        out << "assign " << node_name(d) << " = "
            << node_name(mem) << "[" << node_name(addr) << "];\n";
    }

    static void gen_rst(std::ostream &out, nodeptr d, std::string reset_name)
    {
        out << "assign " << node_name(d) << " = " << reset_name << ";\n";
    }

    static void gen_log2(std::ostream &out, nodeptr d, nodeptr s)
    {
        // This is tricky. There's no easy builtin way of doing this in verilog
        // (well there is, but it's not synthesizable).
        // What we'll do is build a priority encoder using a series of muxes.

        std::string name = node_name(s);
        // The "default" value is 0, now that CHISEL has been corrected
        // to consider log2(1) == 0
        std::string expr = std::to_string(d->width()) + "'d0";
        size_t width = s->width();

        for (size_t i = 1; i < width; i++) {
            expr = "(" + name + "[" + std::to_string(i) + "]) ? "
                 + std::to_string(d->width()) + "'d" + std::to_string(i)
                 + " : (" + expr + ")";
        }

        out << "assign " << node_name(d) << " = " << expr << ";\n";
    }

    static void gen_wire(std::ostream &out, opptr op, std::string reset_name)
    {
        switch (op->op()) {
        case opcode::ADD:
            gen_bin_op(out, "+", op->d(), op->s(), op->t());
            break;
        case opcode::SUB:
            gen_bin_op(out, "-", op->d(), op->s(), op->t());
            break;
        case opcode::MUL:
            gen_bin_op(out, "*", op->d(), op->s(), op->t());
            break;
        case opcode::DIV:
            gen_bin_op(out, "/", op->d(), op->s(), op->t());
            break;
        case opcode::AND:
            gen_bin_op(out, "&", op->d(), op->s(), op->t());
            break;
        case opcode::OR:
            gen_bin_op(out, "|", op->d(), op->s(), op->t());
            break;
        case opcode::XOR:
            gen_bin_op(out, "^", op->d(), op->s(), op->t());
            break;
        case opcode::LSH:
            gen_lshift(out, op->d(), op->s(), op->t());
            break;
        case opcode::RSH:
        case opcode::RSHD:
            gen_rshift(out, op->d(), op->s(), op->t());
            break;
        case opcode::ARSH:
            gen_bin_op(out, ">>>", op->d(), op->s(), op->t());
            break;
        case opcode::EQ:
            gen_bin_op(out, "==", op->d(), op->s(), op->t());
            break;
        case opcode::GTE:
            gen_bin_op(out, ">=", op->d(), op->s(), op->t());
            break;
        case opcode::LT:
            gen_bin_op(out, "<", op->d(), op->s(), op->t());
            break;
        case opcode::NEQ:
            gen_bin_op(out, "!=", op->d(), op->s(), op->t());
            break;
        case opcode::NEG:
            gen_un_op(out, "-", op->d(), op->s());
            break;
        case opcode::NOT:
            gen_un_op(out, "~", op->d(), op->s());
            break;
        case opcode::LOG2:
            gen_log2(out, op->d(), op->s());
            break;
        case opcode::MOV:
        case opcode::OUT:
            gen_un_op(out, "", op->d(), op->s());
            break;
        case opcode::CAT:
        case opcode::CATD:
            gen_cat(out, op->d(), op->s(), op->t());
            break;
        case opcode::MUX:
            gen_mux(out, op->d(), op->s(), op->t(), op->u());
            break;
        case opcode::RD:
            gen_read(out, op->d(), op->t(), op->u());
            break;
        case opcode::RST:
            gen_rst(out, op->d(), reset_name);
        default:
            break;
        }
    }

    void gen_inout(std::ostream &out, std::string inout, nodeptr dest)
    {
            out << ",\n\t" << inout
                << " [" << (dest->width() - 1) << ":0] "
                << node_name(dest);
    }

    void gen_flo(std::shared_ptr<flo<node, operation<node> > > flof,
                 std::ostream &out)
    {
        auto mod_name = class_name(flof);
        if (mod_name == "") {
            fprintf(stderr, "Could not find class name");
        }

        auto clk_name = mod_name + "_clk";
        auto reset_name = mod_name + "_reset";

        out << "module " << mod_name << " (\n"
            << "\tinput " << clk_name << ",\n"
            << "\tinput " << reset_name;

        // split operations into different categories
        std::vector<opptr> registers;
        std::vector<opptr> writes;
        std::vector<opptr> inits;
        std::vector<opptr> wires;
        std::vector<opptr> outputs;

        // print the ports (inputs and outputs)
        // and sort the categories
        for (const auto& op : flof->operations()) {
            switch (op->op()) {
            // ignore memories
            case opcode::MEM:
                break;
            case opcode::IN:
                gen_inout(out, "input", op->d());
                break;
            case opcode::OUT:
                gen_inout(out, "output", op->d());
                outputs.push_back(op);
                break;
            case opcode::REG:
                registers.push_back(op);
                break;
            case opcode::WR:
                writes.push_back(op);
                break;
            case opcode::INIT:
                inits.push_back(op);
                break;
            default:
                wires.push_back(op);
            }
        }

        out << "\n);\n";

        // generate all the memories first
        for (const auto& node : flof->nodes()) {
            if (!node->is_mem())
                continue;

            gen_mem(out, node);
        }

        for (const auto& op : registers)
            gen_decl(out, "reg", op->d());

        for (const auto& op : wires)
            gen_decl(out, "wire", op->d());

        // generate all the combination statements
        for (const auto& op : wires)
            gen_wire(out, op, reset_name);

        // generate the output assignments
        for (const auto& op : outputs)
            gen_wire(out, op, reset_name);

        out << "initial begin\n";
        for (const auto& op: inits)
            gen_init(out, op->s(), op->t(), op->u());
        out << "end\n";

        out << "always @(posedge " << clk_name << ") begin\n";

        for (const auto& op: registers)
            gen_reg_assign(out, op->d(), op->t());

        for (const auto& op: writes)
            gen_write(out, op->s(), op->t(), op->u(), op->v());

        out << "end\nendmodule\n";
    }

    /* Generate $dumpvars expression for inputs and outputs */
    static void gen_vardump(std::ostream &out,
            std::string mod_name, std::vector<nodeptr> &ports)
    {
        out << "\t$dumpvars(1";
        for (const auto &node : ports)
            out << ", " << mod_name << "." << node_name(node);
        out << ");\n\t";
    }

    void gen_step(std::shared_ptr<flo<node, operation<node> > > flof,
                  std::shared_ptr<libstep::step> stepf, size_t clock_period,
                  std::ostream &out)
    {
        std::string mod_name = class_name(flof);
        std::string clk_name = mod_name + "_clk";
        std::string reset_name = mod_name + "_reset";

        out << "`timescale 1ps/1ps\n"
                  << "module " << mod_name << "_tb();\n";

        std::vector<nodeptr> inputs;
        std::vector<nodeptr> outputs;
        std::vector<nodeptr> ports;

        std::map<std::string, unsigned int> sizemap;

        for (const auto &op : flof->operations()) {
            if (op->op() == opcode::IN) {
                inputs.push_back(op->d());
                ports.push_back(op->d());
                sizemap[node_name(op->d())] = op->d()->width();
            } else if (op->op() == opcode::OUT) {
                outputs.push_back(op->d());
                ports.push_back(op->d());
            }
        }

        const size_t clock_delay = clock_period >> 1;

        out << "reg clk;\nreg reset;\n"
                  << "initial clk = 1'b1;\n"
                  << "always #" << clock_delay << " clk = !clk;\n";

        for (const auto &node : inputs)
            out << "reg [" << (node->width() - 1) << ":0] "
                      << node_name(node) << ";\n";

        for (const auto &node : outputs)
            out << "wire [" << (node->width() - 1) << ":0] "
                      << node_name(node) << ";\n";

        out << mod_name << " " << mod_name << " (\n"
                  << "\t." << clk_name << " (clk),\n"
                  << "\t." << reset_name << " (reset)";

        for (const auto &node : ports) {
            auto name = node_name(node);
            out << ",\n\t" << "." << name << " ("
                      << name << ")";
        }

        out << "\n);\n";

        out << "initial begin\n\t";

        for (const auto &act : stepf->actions()) {
            unsigned int width;

            switch (act->at()) {
            case libstep::action_type::STEP:
                out << "#" << clock_period * act->cycles() << " ";
                break;
            case libstep::action_type::WIRE_POKE:
                width = sizemap[act->signal()];
                out << act->signal() << " <= "
                          << width << "'d" << act->value() << ";\n\t";
                break;
            case libstep::action_type::RESET:
                out << "reset <= 1;\n\t#" << clock_period * act->cycles()
                          << " reset <= 0;\n"
                          << "\t$dumpfile(\"" << mod_name << "-test.vcd\");\n";
                gen_vardump(out, mod_name, ports);
                break;
            case libstep::action_type::QUIT:
                out << "$finish;\n";
                break;
            default:
                break;
            }
        }

        out << "end\nendmodule\n";
    }
}
