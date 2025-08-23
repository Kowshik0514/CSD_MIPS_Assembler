// File: structures.h
// Owner: Person CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Defines the core C++ data structures for all instructions.
// Parser will create these objects.

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>

// Using a map to represent the symbol table (maps label name to its address/ID)
using SymbolTable = std::map<std::string, uint32_t>;

// Base class for all instructions
class Instruction {
public:
    virtual ~Instruction() = default;
    // The emit function will be implemented by each specific instruction type.
    // It takes the symbol table to resolve labels to addresses.
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