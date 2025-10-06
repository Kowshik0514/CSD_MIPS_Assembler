// File: structures.h
// Owner: Rashmitha
// Role: Data Structures for Assembler & Linker

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// Represents a single symbol (label or variable).
struct Symbol
{
    enum class Type { TEXT, DATA };
    enum class Binding { LOCAL, GLOBAL };

    std::string name;
    Type type;
    Binding binding = Binding::LOCAL;
    uint32_t address;
    bool is_defined = false;
};

// Represents an entry in the data section.
struct DataEntry {
    std::string name;
    int32_t value;
};

// Represents a relocation entry. Tells the linker where to patch an address.
struct RelocationEntry {
    uint32_t offset;
    std::string target_symbol;
};

// A container for all parsed information from a single .stkasm file.
struct AssemblyUnit {
    std::vector<std::unique_ptr<class Instruction>> instructions;
    std::vector<DataEntry> data_entries;
    std::vector<Symbol> symbol_table;
};

// Base class for all instructions
class Instruction {
public:
    virtual ~Instruction() = default;
    virtual std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const = 0;
};

// --- Existing Instruction Classes ---
class IConst : public Instruction {
public:
    int32_t value;
    explicit IConst(int32_t val) : value(val) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IAdd : public Instruction {
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class ISub : public Instruction {
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IMul : public Instruction {
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IDiv : public Instruction {
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Ret : public Instruction {
public:
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Jmp : public Instruction {
public:
    std::string label;
    explicit Jmp(const std::string &lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Invoke : public Instruction {
public:
    std::string label;
    uint8_t num_args;
    Invoke(const std::string &lbl, uint8_t args) : label(lbl), num_args(args) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

// --- NEW INSTRUCTION CLASSES FOR LOAD/STORE ---
class IStore : public Instruction {
public:
    std::string var_name;
    explicit IStore(const std::string& name) : var_name(name) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

class ILoad : public Instruction {
public:
    std::string var_name;
    explicit ILoad(const std::string& name) : var_name(name) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};


// Represents a loaded .o file in memory.
class ObjectFile {
public:
    std::vector<uint8_t> code_section;
    std::vector<uint8_t> data_section;
    std::vector<Symbol> symbol_table;
    std::vector<RelocationEntry> relocation_table;

    static ObjectFile read_from(const std::string& filepath);
};

#endif // STRUCTURES_H