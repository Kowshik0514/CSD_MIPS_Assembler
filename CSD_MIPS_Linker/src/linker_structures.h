// File: CSD_MIPS_Linker/src/linker_structures.h
// Role: Defines the core C++ data structures for the Linker.

#ifndef LINKER_STRUCTURES_H
#define LINKER_STRUCTURES_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

// Represents a symbol as defined in the .o file's symbol table.
struct ObjectFileSymbol {
    enum class Type { TEXT, DATA };
    enum class Binding { LOCAL, GLOBAL };

    std::string name;
    Type type;
    Binding binding;
    uint32_t address; // Address is relative to the start of its section in the .o file
};

// Represents a relocation entry from the .o file.
struct RelocationEntry {
    uint32_t offset; // Offset within the code section of THIS file to patch.
    std::string target_symbol;
};

// A complete in-memory representation of a single parsed .o file.
class ParsedObjectFile {
public:
    std::vector<uint8_t> code_section;
    std::vector<uint8_t> data_section;
    std::vector<ObjectFileSymbol> symbol_table;
    std::vector<RelocationEntry> relocation_table;

    // A static factory method to load and parse an object file from disk.
    static ParsedObjectFile from_file(const std::string& filepath);
};

#endif // LINKER_STRUCTURES_H