#include <libflo/node.h++>
#include <libflo/operation.h++>

#include "generation.h++"
#include "helpers.h++"

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
    if (!s->is_const() && s->width() < d->width()) {
        size_t zero_width = d->width() - s->width();
        std::cout << "assign " << node_name(d) << " = {"
                  << zero_width << "'d0, " << node_name(s)
                  << "} << " << node_name(t) << ";\n";
        return;
    }
    gen_bin_op("<<", d, s, t);
}

static void gen_rshift(nodeptr d, nodeptr s, nodeptr t)
{
    // Flo uses right shifts by constants
    // to select bits out of signals.
    // This requires special handling in Verilog
    if (t->is_const()) {
        size_t width = d->width();
        size_t start = std::stoi(t->name());
        std::cout << "assign " << node_name(d) << " = "
                  << node_name(s) << "["
                  << (start + width - 1) << ":" << start << "];\n";
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

static void gen_read(nodeptr d, nodeptr mem, nodeptr addr)
{
    std::cout << "assign " << node_name(d) << " = "
              << node_name(mem) << "[" << node_name(addr) << "];\n";
}

void gen_flo(std::shared_ptr<flo<node, operation<node> > > flof)
{
    auto mod_name = class_name(flof);
    if (mod_name == "") {
        fprintf(stderr, "Could not find class name");
    }

    auto clk_name = mod_name + "__clk";
    auto reset_name = mod_name + "__reset";

    std::cout << "module " << mod_name << " (\n"
              << "\tinput " << clk_name << ",\n"
              << "\tinput " << reset_name;

    // print the ports (inputs and outputs)
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

        std::cout << ",\n\t" << inout << " [" << (dest->width() - 1) << ":0] "
                  << node_name(dest);
    }

    std::cout << "\n);\n";

    // generate all the memories first
    for (const auto& node : flof->nodes()) {
        if (!node->is_mem())
            continue;

        gen_mem(node);
    }

    // split operations into registers and wires
    std::vector<opptr> registers;
    std::vector<opptr> wires;

    for (const auto& op : flof->operations()) {
        // ignore memories
        if (op->op() == opcode::MEM)
            continue;

        if (op->op() == opcode::REG) {
            gen_decl("reg", op->d());
            registers.push_back(op);
        } else if (op->op() == opcode::WR) {
            // write is a special case
            // it needs to be in the clocked section
            // but it does not actually generate a register
            registers.push_back(op);
        } else {
            gen_decl("wire", op->d());
            wires.push_back(op);
        }
    }

    // generate all the combination statements
    for (const auto& op : wires) {
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
        default:
            break;
        }
    }

    std::cout << "always @(posedge " << clk_name << ") begin\n"
              << "\tif (" << reset_name << ") begin\n";
    for (const auto& op: registers) {
        if (op->op() != opcode::REG)
            continue;
        gen_reg_assign(op->d(), op->s());
    }
    std::cout << "\tend else begin\n";
    for (const auto& op: registers) {
        if (op->op() == opcode::WR)
            gen_write(op->s(), op->t(), op->u(), op->v());
        else
            gen_reg_assign(op->d(), op->t());
    }
    std::cout << "\tend\nend\nendmodule\n";
}
