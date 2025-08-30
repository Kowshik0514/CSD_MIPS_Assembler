// File: structures.h
// Owner: Person CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Defines the core C++ data structures for all instructions.

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "symbol_table.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// ** The old "using SymbolTable = std::map<...>" line is now REMOVED. **
// We now use the powerful SymbolTable class defined in symbol_table.h

// Base class for all instructions
class Instruction {
public:
    virtual ~Instruction() = default;
    // The emit function now takes a constant reference to the new SymbolTable class.
    virtual std::vector<uint8_t> emit(const SymbolTable& symbols) const = 0;
};

// Represents an 'iconst <value>' instruction
class IConst : public Instruction {
public:
    int32_t value;
    explicit IConst(int32_t val) : value(val) {}
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Represents an 'iadd' instruction
class IAdd : public Instruction {
public:
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Module 2
// Represents an 'isub' instruction
class ISub : public Instruction {
public:
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Represents an 'imul' instruction
class IMul : public Instruction {
public:
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Represents an 'idiv' instruction
class IDiv : public Instruction {
public:
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Represents a 'jmp <label>' instruction
class Jmp : public Instruction {
public:
    std::string label;
    explicit Jmp(const std::string& lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};


// Represents an 'invoke <label> <num_args>' instruction
class Invoke : public Instruction {
public:
    std::string label;
    uint8_t num_args;
    Invoke(const std::string& lbl, uint8_t args) : label(lbl), num_args(args) {}
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

// Represents a 'ret' instruction
class Ret : public Instruction {
public:
    std::vector<uint8_t> emit(const SymbolTable& symbols) const override;
};

#endif