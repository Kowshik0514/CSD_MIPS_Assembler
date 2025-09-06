// File: emitter.cpp
// Owner: CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Implementation of the object file emitter.

#include "emitter.h"
#include "structures.h"
#include <stdexcept>
#include <algorithm>

// --- Helper functions ---
void write_int32(std::vector<uint8_t> &vec, int32_t value) {
    for (int i = 0; i < 4; i++) {
        vec.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

void write_string(std::vector<uint8_t> &vec, const std::string &str) {
    write_int32(vec, static_cast<int32_t>(str.length()));
    vec.insert(vec.end(), str.begin(), str.end());
}

// --- Opcodes ---
enum class Opcode : uint8_t {
    ICONST = 0x01,
    IADD   = 0x02,
    ISUB   = 0x03,
    IMUL   = 0x04,
    IDIV   = 0x05,
    RET    = 0x06,
    JMP    = 0x07,
    INVOKE = 0x08
};

// --- Symbol lookup ---
const Symbol* find_symbol(const AssemblyUnit &unit, const std::string &name) {
    for (const auto &sym : unit.symbol_table) {
        if (sym.name == name) return &sym;
    }
    return nullptr;
}

// --- Instruction implementations ---
std::vector<uint8_t> IConst::emit(const AssemblyUnit &, RelocationEntry &) const {
    std::vector<uint8_t> code;
    code.push_back(static_cast<uint8_t>(Opcode::ICONST));
    write_int32(code, value);
    return code;
}

std::vector<uint8_t> IAdd::emit(const AssemblyUnit &, RelocationEntry &) const {
    return { static_cast<uint8_t>(Opcode::IADD) };
}

std::vector<uint8_t> ISub::emit(const AssemblyUnit &, RelocationEntry &) const {
    return { static_cast<uint8_t>(Opcode::ISUB) };
}

std::vector<uint8_t> IMul::emit(const AssemblyUnit &, RelocationEntry &) const {
    return { static_cast<uint8_t>(Opcode::IMUL) };
}

std::vector<uint8_t> IDiv::emit(const AssemblyUnit &, RelocationEntry &) const {
    return { static_cast<uint8_t>(Opcode::IDIV) };
}

std::vector<uint8_t> Ret::emit(const AssemblyUnit &, RelocationEntry &) const {
    return { static_cast<uint8_t>(Opcode::RET) };
}

std::vector<uint8_t> Jmp::emit(const AssemblyUnit &unit, RelocationEntry &reloc) const {
    std::vector<uint8_t> code;
    code.push_back(static_cast<uint8_t>(Opcode::JMP));
    const Symbol *target = find_symbol(unit, label);
    if (!target) {
        throw std::runtime_error("Undefined symbol: " + label);
    }
    if (target->binding == Symbol::Binding::LOCAL) {
        write_int32(code, target->address);
    } else {
        write_int32(code, 0); // placeholder
        reloc.target_symbol = label;
    }
    return code;
}

std::vector<uint8_t> Invoke::emit(const AssemblyUnit &unit, RelocationEntry &reloc) const {
    std::vector<uint8_t> code;
    code.push_back(static_cast<uint8_t>(Opcode::INVOKE));
    const Symbol *target = find_symbol(unit, label);
    if (!target) {
        throw std::runtime_error("Undefined symbol: " + label);
    }
    if (target->binding == Symbol::Binding::LOCAL) {
        write_int32(code, target->address);
    } else {
        write_int32(code, 0); // placeholder
        reloc.target_symbol = label;
    }
    code.push_back(num_args);
    return code;
}

// --- Main Emitter Function ---
std::vector<uint8_t> emit_object_file(const AssemblyUnit &unit) {
    std::vector<uint8_t> code_section;
    std::vector<RelocationEntry> relocation_table;

    // 1. Generate Code Section
    for (const auto &instr : unit.instructions) {
        RelocationEntry reloc_entry;
        uint32_t offset = code_section.size();
        std::vector<uint8_t> instr_bytes = instr->emit(unit, reloc_entry);
        code_section.insert(code_section.end(), instr_bytes.begin(), instr_bytes.end());
        if (!reloc_entry.target_symbol.empty()) {
            reloc_entry.offset = offset + 1; // address after opcode
            relocation_table.push_back(reloc_entry);
        }
    }

    // 2. Generate Data Section
    std::vector<uint8_t> data_section;
    for (const auto &data : unit.data_entries) {
        write_int32(data_section, data.value);
    }

    // 3. Generate Symbol Table Section
    std::vector<uint8_t> symbol_table_section;
    write_int32(symbol_table_section, unit.symbol_table.size());
    for (const auto &sym : unit.symbol_table) {
        write_string(symbol_table_section, sym.name);
        symbol_table_section.push_back(static_cast<uint8_t>(sym.type));
        symbol_table_section.push_back(static_cast<uint8_t>(sym.binding));
        write_int32(symbol_table_section, sym.address);
    }

    // 4. Generate Relocation Table Section
    std::vector<uint8_t> reloc_table_section;
    write_int32(reloc_table_section, relocation_table.size());
    for (const auto &reloc : relocation_table) {
        write_int32(reloc_table_section, reloc.offset);
        write_string(reloc_table_section, reloc.target_symbol);
    }

    // 5. Stitch everything together
    std::vector<uint8_t> object_file;
    uint32_t magic_number = 0x5354414F; // "STAO"
    write_int32(object_file, magic_number);
    write_int32(object_file, code_section.size());
    write_int32(object_file, data_section.size());
    write_int32(object_file, symbol_table_section.size());
    write_int32(object_file, reloc_table_section.size());

    object_file.insert(object_file.end(), code_section.begin(), code_section.end());
    object_file.insert(object_file.end(), data_section.begin(), data_section.end());
    object_file.insert(object_file.end(), symbol_table_section.begin(), symbol_table_section.end());
    object_file.insert(object_file.end(), reloc_table_section.begin(), reloc_table_section.end());

    return object_file;
}
