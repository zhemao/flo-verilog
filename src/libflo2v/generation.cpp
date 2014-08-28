#include "generation.hpp"
#include "helpers.hpp"

#include <iostream>

using namespace libflo;

static void gen_bin_op(std::string op, nodeptr d, nodeptr s, nodeptr t)
{
    std::cout << "assign " << node_name(d) << " = "
              << node_name(s) << " " << op << " " << node_name(t) << ";\n";
}

static void gen_un_op(std::string op, nodeptr d, nodeptr s)
{
    std::cout << "assign " << node_name(d) << " = " << op
              << node_name(s) << ";\n";
}

static void gen_mem(nodeptr node)
{
    std::cout << "reg [" << (node->width() - 1) << ":0] "
              << node_name(node) << " ["
              << (node->depth() - 1) << ":0];\n";
}

static void gen_lshift(nodeptr d, nodeptr s, nodeptr t)
{
    if (s->width() < d->width()) {
        size_t zero_width = d->width() - s->width();
        std::cout << "assign " << node_name(d) << " = {"
                  << zero_width << "'d0, " << node_name(s)
                  << "} << " << node_name(t) << ";\n";
        return;
    }
    if (s->width() > d->width()) {
        std::cout << "assign " << node_name(d) << " = "
                  << node_name(s) << "[" << (d->width() - 1)
                  << ":0] << " << node_name(t) << ";\n";
        return;
    }
    gen_bin_op("<<", d, s, t);
}

static void gen_selection(nodeptr d, nodeptr s, nodeptr t)
{
    size_t width = d->width();
    size_t start = std::stoi(t->name());
    size_t highest = start + width;

    if (highest > s->width()) {
        size_t extend = highest - s->width();
        std::cout << "assign " << node_name(d) << " = "
            << "{" << extend << "'d0, "
            << node_name(s) << "[" << (s->width() - 1) << ":"
            << start << "]};\n";
        return;
    }

    std::cout << "assign " << node_name(d) << " = "
              << node_name(s) << "["
              << (highest - 1) << ":" << start << "];\n";
}

static void gen_rshift(nodeptr d, nodeptr s, nodeptr t)
{
    // Flo uses right shifts by constants
    // to select bits out of signals.
    // This requires special handling in Verilog
    if (t->is_const()) {
        gen_selection(d, s, t);
        return;
    }

    if (s->width() < d->width()) {
        size_t zero_width = d->width() - s->width();
        std::cout << "assign " << node_name(d) << " = {"
                  << zero_width << "'d0, " << node_name(s)
                  << "} >> " << node_name(t) << ";\n";
        return;
    }

    gen_bin_op(">>", d, s, t);
}

static void gen_cat(nodeptr d, nodeptr s, nodeptr t)
{
    std::cout << "assign " << node_name(d) << " = {"
              << node_name(s) << ", " << node_name(t) << "};\n";
}

static void gen_decl(std::string typ, nodeptr d)
{
    std::cout << typ << " [" << (d->width() - 1) << ":0] "
              << node_name(d) << ";\n";
}

static void gen_reg_assign(nodeptr reg, nodeptr val)
{
    std::cout << "\t\t" << node_name(reg) << " <= " << node_name(val) << ";\n";
}

static void gen_mux(nodeptr d, nodeptr s, nodeptr t, nodeptr u)
{
    std::cout << "assign " << node_name(d) << " = " << "(" << node_name(s)
              << ") ? " << node_name(t) << " : " << node_name(u) << ";\n";
}

static void gen_write(nodeptr en, nodeptr mem, nodeptr addr, nodeptr val)
{
    std::cout << "\t\tif (" << node_name(en) << ") "
              << node_name(mem) << "[" << node_name(addr) << "] <= "
              << node_name(val) << ";\n";
}

static void gen_init(nodeptr mem, nodeptr addr, nodeptr val)
{
    std::cout << "\t\t" << node_name(mem) << "["
                // don't use node_name for addr, otherwise it will
                // try to put the wrong width on it
              << addr->name() << "] <= " << node_name(val) << ";\n";
}

static void gen_read(nodeptr d, nodeptr mem, nodeptr addr)
{
    std::cout << "assign " << node_name(d) << " = "
              << node_name(mem) << "[" << node_name(addr) << "];\n";
}

static void gen_rst(nodeptr d, std::string reset_name)
{
    std::cout << "assign " << node_name(d) << " = " << reset_name << ";\n";
}

static void gen_log2(nodeptr d, nodeptr s)
{
    // This is tricky. There's no easy builtin way of doing this in verilog
    // (well there is, but it's not synthesizable).
    // What we'll do is build a priority encoder using a series of muxes.

    std::string name = node_name(s);
    // 1 is the smallest number that log2 can be
    // (Chisel considers log2(1) = 1)
    std::string expr = std::to_string(d->width()) + "'d1";
    size_t width = s->width();

    for (size_t i = 2; i < width; i++) {
        expr = "(" + name + "[" + std::to_string(i) + "]) ? "
             + std::to_string(d->width()) + "'d" + std::to_string(i)
             + " : (" + expr + ")";
    }

    std::cout << "assign " << node_name(d) << " = " << expr << ";\n";
}

static void gen_wire(opptr op, std::string reset_name)
{
    switch (op->op()) {
    case opcode::ADD:
        gen_bin_op("+", op->d(), op->s(), op->t());
        break;
    case opcode::SUB:
        gen_bin_op("-", op->d(), op->s(), op->t());
        break;
    case opcode::MUL:
        gen_bin_op("*", op->d(), op->s(), op->t());
        break;
    case opcode::DIV:
        gen_bin_op("/", op->d(), op->s(), op->t());
        break;
    case opcode::AND:
        gen_bin_op("&", op->d(), op->s(), op->t());
        break;
    case opcode::OR:
        gen_bin_op("|", op->d(), op->s(), op->t());
        break;
    case opcode::XOR:
        gen_bin_op("^", op->d(), op->s(), op->t());
        break;
    case opcode::LSH:
        gen_lshift(op->d(), op->s(), op->t());
        break;
    case opcode::RSH:
    case opcode::RSHD:
        gen_rshift(op->d(), op->s(), op->t());
        break;
    case opcode::ARSH:
        gen_bin_op(">>>", op->d(), op->s(), op->t());
        break;
    case opcode::EQ:
        gen_bin_op("==", op->d(), op->s(), op->t());
        break;
    case opcode::GTE:
        gen_bin_op(">=", op->d(), op->s(), op->t());
        break;
    case opcode::LT:
        gen_bin_op("<", op->d(), op->s(), op->t());
        break;
    case opcode::NEQ:
        gen_bin_op("!=", op->d(), op->s(), op->t());
        break;
    case opcode::NEG:
        gen_un_op("-", op->d(), op->s());
        break;
    case opcode::NOT:
        gen_un_op("~", op->d(), op->s());
        break;
    case opcode::LOG2:
        gen_log2(op->d(), op->s());
        break;
    case opcode::MOV:
    case opcode::OUT:
        gen_un_op("", op->d(), op->s());
        break;
    case opcode::CAT:
    case opcode::CATD:
        gen_cat(op->d(), op->s(), op->t());
        break;
    case opcode::MUX:
        gen_mux(op->d(), op->s(), op->t(), op->u());
        break;
    case opcode::RD:
        gen_read(op->d(), op->t(), op->u());
        break;
    case opcode::RST:
        gen_rst(op->d(), reset_name);
    default:
        break;
    }
}

void gen_inout(std::string inout, nodeptr dest)
{
        std::cout << ",\n\t" << inout
                  << " [" << (dest->width() - 1) << ":0] "
                  << node_name(dest);
}

void gen_flo(std::shared_ptr<flo<node, operation<node> > > flof)
{
    auto mod_name = class_name(flof);
    if (mod_name == "") {
        fprintf(stderr, "Could not find class name");
    }

    auto clk_name = mod_name + "_clk";
    auto reset_name = mod_name + "_reset";

    std::cout << "module " << mod_name << " (\n"
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
            gen_inout("input", op->d());
            break;
        case opcode::OUT:
            gen_inout("output", op->d());
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

    std::cout << "\n);\n";

    // generate all the memories first
    for (const auto& node : flof->nodes()) {
        if (!node->is_mem())
            continue;

        gen_mem(node);
    }

    for (const auto& op : registers)
        gen_decl("reg", op->d());

    for (const auto& op : wires)
        gen_decl("wire", op->d());

    // generate all the combination statements
    for (const auto& op : wires)
        gen_wire(op, reset_name);

    // generate the output assignments
    for (const auto& op : outputs)
        gen_wire(op, reset_name);

    std::cout << "always @(posedge " << clk_name << ") begin\n"
              << "\tif (" << reset_name << ") begin\n";

    for (const auto& op: registers) {
        if (op->op() != opcode::REG)
            continue;
        gen_reg_assign(op->d(), op->s());
    }

    for (const auto& op: inits)
        gen_init(op->s(), op->t(), op->u());

    std::cout << "\tend else begin\n";

    for (const auto& op: registers)
        gen_reg_assign(op->d(), op->t());

    for (const auto& op: writes)
        gen_write(op->s(), op->t(), op->u(), op->v());

    std::cout << "\tend\nend\nendmodule\n";
}

/* Generate $dumpvars expression for inputs and outputs */
static void gen_vardump(std::vector<nodeptr> &ports)
{
    std::cout << "\t$dumpvars(1";
    for (const auto &node : ports)
        std::cout << ", " << node_name(node);
    std::cout << ");\n\t";
}

void gen_step(std::shared_ptr<flo<node, operation<node> > > flof,
              std::shared_ptr<libstep::step> stepf, size_t clock_period)
{
    std::string mod_name = class_name(flof);
    std::string clk_name = mod_name + "_clk";
    std::string reset_name = mod_name + "_reset";

    std::cout << "`timescale 1ps/1ps\n"
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

    std::cout << "reg clk;\nreg reset;\n"
              << "initial clk = 1'b1;\n"
              << "always #" << clock_delay << " clk = !clk;\n";

    for (const auto &node : inputs)
        std::cout << "reg [" << (node->width() - 1) << ":0] "
                  << node_name(node) << ";\n";

    for (const auto &node : outputs)
        std::cout << "wire [" << (node->width() - 1) << ":0] "
                  << node_name(node) << ";\n";

    std::cout << mod_name << " test (\n"
              << "\t." << clk_name << " (clk),\n"
              << "\t." << reset_name << " (reset)";

    for (const auto &node : ports) {
        auto name = node_name(node);
        std::cout << ",\n\t" << "." << name << " ("
                  << name << ")";
    }

    std::cout << "\n);\n";

    std::cout << "initial begin\n\t";

    for (const auto &act : stepf->actions()) {
        unsigned int width;

        switch (act->at()) {
        case libstep::action_type::STEP:
            std::cout << "#" << clock_period * act->cycles() << " ";
            break;
        case libstep::action_type::WIRE_POKE:
            width = sizemap[act->signal()];
            std::cout << act->signal() << " = "
                      << width << "'d" << act->value() << ";\n\t";
            break;
        case libstep::action_type::RESET:
            std::cout << "reset = 1;\n\t#" << clock_period * act->cycles()
                      << " reset = 0;\n"
                      << "\t$dumpfile(\"" << mod_name << "-test.vcd\");\n";
            gen_vardump(ports);
            break;
        case libstep::action_type::QUIT:
            std::cout << "$finish;\n";
            break;
        default:
            break;
        }
    }

    std::cout << "end\nendmodule\n";
}
