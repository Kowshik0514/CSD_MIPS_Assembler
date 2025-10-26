// File: structures.h
// Owner: Rashmitha
// Role: Data Structures for Assembler & Linker

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>

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

// Represents a relocation entry.
struct RelocationEntry {
    uint32_t offset;
    std::string target_symbol;
};

// Container for parsed assembly file.
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

// --- Instruction Classes with INLINE CONSTRUCTORS ---
class IConst : public Instruction {
public:
    int32_t value;
    // Define constructor inline
    explicit IConst(int32_t val) : value(val) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IAdd : public Instruction {
public:
    // Default constructor is fine
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class ISub : public Instruction {
public:
    // Default constructor is fine
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IMul : public Instruction {
public:
    // Default constructor is fine
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IDiv : public Instruction {
public:
    // Default constructor is fine
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Ret : public Instruction {
public:
    // Default constructor is fine
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Jmp : public Instruction {
public:
    std::string label;
    // Define constructor inline
    explicit Jmp(const std::string &lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class Invoke : public Instruction {
public:
    std::string label;
    uint8_t num_args;
    // Define constructor inline
    Invoke(const std::string &lbl, uint8_t args) : label(lbl), num_args(args) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class IStore : public Instruction {
public:
    std::string var_name;
    // Define constructor inline
    explicit IStore(const std::string& name) : var_name(name) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class ILoad : public Instruction {
public:
    std::string var_name;
    // Define constructor inline
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

// Global Symbol Table Alias
using GlobalSymbolTable = std::map<std::string, uint32_t>;

#endif // STRUCTURES_H