// File: structures.h
// Owner: Rashmitha
// Role: Data Structures for Assembler & Linker
// Description: Defines all instruction classes and data structures. (Fully Updated)

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>

// --- Core Data Structures ---

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

struct DataEntry {
    std::string name;
    int32_t value;
};

struct RelocationEntry {
    uint32_t offset;
    std::string target_symbol;
};

struct AssemblyUnit {
    std::vector<std::unique_ptr<class Instruction>> instructions;
    std::vector<DataEntry> data_entries;
    std::vector<Symbol> symbol_table;
};

// --- Base Instruction Class ---
class Instruction {
public:
    std::string source_line;
    virtual ~Instruction() = default;
    virtual std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const = 0;
};

// --- Instruction Classes ---

// STACK
class IConst : public Instruction {
public:
    int32_t value;
    explicit IConst(int32_t val) : value(val) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

// ARITHMETIC
class IAdd : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class ISub : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class IMul : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class IDiv : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };

// CONTROL FLOW
class Ret : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
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

// LOCAL VARIABLES
class IStore : public Instruction {
public:
    int32_t index;
    explicit IStore(int32_t idx) : index(idx) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
class ILoad : public Instruction {
public:
    int32_t index;
    explicit ILoad(int32_t idx) : index(idx) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

// ARRAYS
class NewArray : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class SetElem : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class GetElem : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };

// STRINGS
class NewString : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class SetChar : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class GetChar : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };

// CONDITIONALS
class ICmpEQ : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class ICmpLT : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class ICmpGT : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class JmpIfFalse : public Instruction {
public:
    std::string label;
    explicit JmpIfFalse(const std::string &lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};
// NEW: JNZ
class JmpIfNotZero : public Instruction {
public:
    std::string label;
    explicit JmpIfNotZero(const std::string &lbl) : label(lbl) {}
    std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override;
};

// I/O
class PrintI : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };
class PrintS : public Instruction { public: std::vector<uint8_t> emit(const AssemblyUnit &unit, RelocationEntry &reloc) const override; };


// --- ObjectFile Class (for Linker) ---
class ObjectFile {
public:
    std::vector<uint8_t> code_section;
    std::vector<uint8_t> data_section;
    std::vector<Symbol> symbol_table;
    std::vector<RelocationEntry> relocation_table;

    static ObjectFile read_from(const std::string& filepath);
};

// --- Global Symbol Table Alias ---
using GlobalSymbolTable = std::map<std::string, uint32_t>;

#endif // STRUCTURES_H