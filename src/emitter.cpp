// File: emitter.cpp
// Owner: CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Implementation of the bytecode emitter and instruction emit methods.

#include "emitter.h"
#include <stdexcept>

// --- Helper function to write a 4-byte integer in little-endian format ---
void write_int32(std::vector<uint8_t>& vec, int32_t value) {
    vec.push_back((value >> 0)  & 0xFF);
    vec.push_back((value >> 8)  & 0xFF);
    vec.push_back((value >> 16) & 0xFF);
    vec.push_back((value >> 24) & 0xFF);
}

// --- Opcode Enum (Updated for Module 2) ---
enum class Opcode : uint8_t {
    ICONST = 0x01,
    IADD   = 0x02,
    INVOKE = 0x03,
    RET    = 0x04,
    // MODULE 2 
    ISUB   = 0x05,
    IMUL   = 0x06,
    IDIV   = 0x07,
    JMP    = 0x08,
};

// --- Implementation of emit() for existing instruction classes ---

std::vector<uint8_t> IConst::emit(const SymbolTable& symbols) const {
    std::vector<uint8_t> bytecode;
    bytecode.push_back(static_cast<uint8_t>(Opcode::ICONST));
    write_int32(bytecode, this->value);
    return bytecode;
}

std::vector<uint8_t> IAdd::emit(const SymbolTable& symbols) const {
    return { static_cast<uint8_t>(Opcode::IADD) };
}

std::vector<uint8_t> Ret::emit(const SymbolTable& symbols) const {
    return { static_cast<uint8_t>(Opcode::RET) };
}


std::vector<uint8_t> Invoke::emit(const SymbolTable& symbols) const {
    std::vector<uint8_t> bytecode;
    bytecode.push_back(static_cast<uint8_t>(Opcode::INVOKE));

    // Look up the label using the new, safer get_address method.
    auto address_opt = symbols.get_address(this->label);
    if (!address_opt) {
        // If the optional is empty, the symbol was not found.
        throw std::runtime_error("Undefined symbol referenced: " + this->label);
    }
    
    // Dereference the optional with * to get the address and write it.
    write_int32(bytecode, *address_opt);
    bytecode.push_back(this->num_args);
    
    return bytecode;
}

// MODULE 2

std::vector<uint8_t> ISub::emit(const SymbolTable& symbols) const {
    return { static_cast<uint8_t>(Opcode::ISUB) };
}

std::vector<uint8_t> IMul::emit(const SymbolTable& symbols) const {
    return { static_cast<uint8_t>(Opcode::IMUL) };
}

std::vector<uint8_t> IDiv::emit(const SymbolTable& symbols) const {
    return { static_cast<uint8_t>(Opcode::IDIV) };
}

// ** NEW and uses the new SymbolTable class **
std::vector<uint8_t> Jmp::emit(const SymbolTable& symbols) const {
    std::vector<uint8_t> bytecode;
    bytecode.push_back(static_cast<uint8_t>(Opcode::JMP));

    // Look up the label using the new get_address method.
    auto address_opt = symbols.get_address(this->label);
    if (!address_opt) {
        throw std::runtime_error("Undefined symbol referenced: " + this->label);
    }
    
    // Dereference the optional to get the address and write it.
    write_int32(bytecode, *address_opt);
    
    return bytecode;
}

// --- Implementation of the main emitter function ---

std::vector<uint8_t> emit_bytecode(
    const std::vector<std::unique_ptr<Instruction>>& instructions,
    const SymbolTable& symbols) {
    
    std::vector<uint8_t> bytecode;

    // --- Write Header ---
    uint32_t magic_number = 0x5354414B; // "STAK"
    uint32_t instruction_count = instructions.size();
    write_int32(bytecode, magic_number);
    write_int32(bytecode, instruction_count);

    for (const auto& instr : instructions) {
        std::vector<uint8_t> instr_bytes = instr->emit(symbols);
        bytecode.insert(bytecode.end(), instr_bytes.begin(), instr_bytes.end());
    }

    return bytecode;
}