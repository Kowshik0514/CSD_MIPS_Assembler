#include "emitter.h"
#include "structures.h"
#include <stdexcept>
#include <vector>

void write_int32(std::vector<uint8_t> &vec, int32_t value) {
    vec.push_back(static_cast<uint8_t>((value >> 0)  & 0xFF));
    vec.push_back(static_cast<uint8_t>((value >> 8)  & 0xFF));
    vec.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    vec.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}
void write_string(std::vector<uint8_t> &vec, const std::string &str) {
    write_int32(vec, static_cast<int32_t>(str.length()));
    vec.insert(vec.end(), str.begin(), str.end());
}

enum class Opcode : uint8_t {
    ICONST = 0x01,
    IADD   = 0x02,
    ISUB   = 0x03,
    IMUL   = 0x04,
    IDIV   = 0x05,
    RET    = 0x06,
    JMP    = 0x07,
    INVOKE = 0x08,
    ISTORE = 0x09,
    ILOAD  = 0x0A,
    
    NEW_ARRAY = 0x10,
    SET_ELEM  = 0x11,
    GET_ELEM  = 0x12,
    
    NEW_STRING = 0x13,
    SET_CHAR   = 0x14,
    GET_CHAR   = 0x15,
    
    ICMP_EQ = 0x20,
    ICMP_LT = 0x21,
    ICMP_GT = 0x22,
    JMP_IF_FALSE = 0x23,
    JNZ          = 0x24, // <-- ADDED JNZ
    
    PRINT_I = 0x30,
    PRINT_S = 0x31
};

const Symbol* find_symbol(const AssemblyUnit &unit, const std::string &name) {
    for (const auto &sym : unit.symbol_table) { if (sym.name == name) return &sym; }
    return nullptr;
}

// Helpers
static std::vector<uint8_t> emit_jump(uint8_t op, const AssemblyUnit &u, RelocationEntry &r, const std::string& l) {
    std::vector<uint8_t> c; c.push_back(op);
    const Symbol *t = find_symbol(u, l);
    if (t && t->is_defined && t->binding == Symbol::Binding::LOCAL) write_int32(c, t->address);
    else { write_int32(c, 0); r.target_symbol = l; }
    return c;
}

// Implementations
std::vector<uint8_t> IConst::emit(const AssemblyUnit&, RelocationEntry&) const { std::vector<uint8_t> c; c.push_back((uint8_t)Opcode::ICONST); write_int32(c, value); return c; }
std::vector<uint8_t> IAdd::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::IADD}; }
std::vector<uint8_t> ISub::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::ISUB}; }
std::vector<uint8_t> IMul::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::IMUL}; }
std::vector<uint8_t> IDiv::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::IDIV}; }
std::vector<uint8_t> Ret::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::RET}; }
std::vector<uint8_t> Jmp::emit(const AssemblyUnit& u, RelocationEntry& r) const { return emit_jump((uint8_t)Opcode::JMP, u, r, label); }
std::vector<uint8_t> JmpIfFalse::emit(const AssemblyUnit& u, RelocationEntry& r) const { return emit_jump((uint8_t)Opcode::JMP_IF_FALSE, u, r, label); }
std::vector<uint8_t> JmpIfNotZero::emit(const AssemblyUnit& u, RelocationEntry& r) const { return emit_jump((uint8_t)Opcode::JNZ, u, r, label); }
std::vector<uint8_t> Invoke::emit(const AssemblyUnit& u, RelocationEntry& r) const { std::vector<uint8_t> c = emit_jump((uint8_t)Opcode::INVOKE, u, r, label); c.push_back(num_args); return c; }
std::vector<uint8_t> IStore::emit(const AssemblyUnit&, RelocationEntry&) const { std::vector<uint8_t> c; c.push_back((uint8_t)Opcode::ISTORE); write_int32(c, index); return c; }
std::vector<uint8_t> ILoad::emit(const AssemblyUnit&, RelocationEntry&) const { std::vector<uint8_t> c; c.push_back((uint8_t)Opcode::ILOAD); write_int32(c, index); return c; }
std::vector<uint8_t> NewArray::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::NEW_ARRAY}; }
std::vector<uint8_t> SetElem::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::SET_ELEM}; }
std::vector<uint8_t> GetElem::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::GET_ELEM}; }
std::vector<uint8_t> NewString::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::NEW_STRING}; }
std::vector<uint8_t> SetChar::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::SET_CHAR}; }
std::vector<uint8_t> GetChar::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::GET_CHAR}; }
std::vector<uint8_t> ICmpEQ::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::ICMP_EQ}; }
std::vector<uint8_t> ICmpLT::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::ICMP_LT}; }
std::vector<uint8_t> ICmpGT::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::ICMP_GT}; }
std::vector<uint8_t> PrintI::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::PRINT_I}; }
std::vector<uint8_t> PrintS::emit(const AssemblyUnit&, RelocationEntry&) const { return {(uint8_t)Opcode::PRINT_S}; }

// Main
std::vector<uint8_t> emit_object_file(const AssemblyUnit &unit) {
    std::vector<uint8_t> code;
    std::vector<RelocationEntry> relocs;
    for (const auto &i : unit.instructions) {
        RelocationEntry r;
        uint32_t off = code.size();
        std::vector<uint8_t> b = i->emit(unit, r);
        code.insert(code.end(), b.begin(), b.end());
        if (!r.target_symbol.empty()) { r.offset = off + 1; relocs.push_back(r); }
    }
    std::vector<uint8_t> data;
    for (const auto &d : unit.data_entries) write_int32(data, d.value);
    
    std::vector<uint8_t> syms;
    write_int32(syms, unit.symbol_table.size());
    for (const auto &s : unit.symbol_table) {
        write_string(syms, s.name);
        syms.push_back((uint8_t)s.type);
        syms.push_back((uint8_t)s.binding);
        syms.push_back((uint8_t)s.is_defined);
        write_int32(syms, s.address);
    }
    std::vector<uint8_t> r_sec;
    write_int32(r_sec, relocs.size());
    for (const auto &r : relocs) { write_int32(r_sec, r.offset); write_string(r_sec, r.target_symbol); }

    std::vector<uint8_t> obj;
    write_int32(obj, 0x5354414F);
    write_int32(obj, code.size()); write_int32(obj, data.size());
    write_int32(obj, syms.size()); write_int32(obj, r_sec.size());
    obj.insert(obj.end(), code.begin(), code.end());
    obj.insert(obj.end(), data.begin(), data.end());
    obj.insert(obj.end(), syms.begin(), syms.end());
    obj.insert(obj.end(), r_sec.begin(), r_sec.end());
    return obj;
}