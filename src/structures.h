// File: structures.h
// Owner: Rashmitha
// Role: Data Structures for Assembler & Linker
// Description: Defines the core C++ data structures for all instructions, directives,
//              and the components of a relocatable object file (.o).

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>


// Represents a single symbol (label or variable).
struct Symbol
{
    enum class Type
    {
        TEXT,
        DATA
    };
    enum class Binding
    {
        LOCAL,
        GLOBAL
    };

    std::string name;
    Type type;
    Binding binding = Binding::LOCAL; // Symbols are local by default.
    uint32_t address;                 // Address within its section.
};

// Represents an entry in the data section.
struct DataEntry
{
    std::string name;
    int32_t value;
};

// Represents a relocation entry. Tells the linker where to patch an address.
struct RelocationEntry
{
    uint32_t offset;           // Byte offset in the code section that needs patching.
    std::string target_symbol; // The global symbol whose address should be patched in.
};

// A container for all the parsed information from a single .stkasm file.
struct AssemblyUnit
{
    std::vector<std::unique_ptr<class Instruction>> instructions;
    std::vector<DataEntry> data_entries;
    std::vector<Symbol> symbol_table;
};

// Base class for all instructions
class Instruction
{
public:
    virtual ~Instruction() = default;
    // The emit function now helps generate relocation entries.
    virtual std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const = 0;
};

// Instruction Classes (IConst, IAdd, ISub, IMul, IDiv, Ret, Jmp, Invoke)
// ... class definitions are the same as in the previous response ...
class IConst : public Instruction
{
public:
    int32_t value;
    explicit IConst(int32_t val) : value(val) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IAdd : public Instruction
{
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class ISub : public Instruction
{
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IMul : public Instruction
{
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IDiv : public Instruction
{
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Ret : public Instruction
{
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Jmp : public Instruction
{
public:
    std::string label;
    explicit Jmp(const std::string &lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Invoke : public Instruction
{
public:
    std::string label;
    uint8_t num_args;
    Invoke(const std::string &lbl, uint8_t args) : label(lbl), num_args(args) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

#endif // STRUCTURES_H