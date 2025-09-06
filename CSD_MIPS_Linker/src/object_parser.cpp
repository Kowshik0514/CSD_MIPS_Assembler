// File: CSD_MIPS_Linker/src/object_parser.cpp
// Role: Implements the logic to parse a .o binary file.

#include "linker_structures.h"
#include <fstream>
#include <stdexcept>

// Reads a 4-byte little-endian integer from the file stream.
int32_t read_int32(std::ifstream& file) {
    int32_t value = 0;
    file.read(reinterpret_cast<char*>(&value), 4);
    if (file.gcount() != 4) throw std::runtime_error("Unexpected end of file while reading integer.");
    return value;
}

// Reads a length-prefixed string from the file stream.
std::string read_string(std::ifstream& file) {
    int32_t len = read_int32(file);
    if (len < 0 || len > 1024) throw std::runtime_error("Invalid string length in object file.");
    std::string str(len, '\0');
    file.read(&str[0], len);
    if (file.gcount() != len) throw std::runtime_error("Unexpected end of file while reading string.");
    return str;
}

// Main Parsing Function: Implementation of the static method from the header.
ParsedObjectFile ParsedObjectFile::from_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open object file: " + filepath);
    }

    ParsedObjectFile obj;

    // 1. Parse Header
    uint32_t magic = read_int32(file);
    if (magic != 0x5354414F) { // "STAO"
        throw std::runtime_error("Invalid object file format: incorrect magic number in " + filepath);
    }
    uint32_t code_size = read_int32(file);
    uint32_t data_size = read_int32(file);
    uint32_t symbol_table_size = read_int32(file);
    uint32_t reloc_table_size = read_int32(file);

    // 2. Parse Code Section
    obj.code_section.resize(code_size);
    file.read(reinterpret_cast<char*>(obj.code_section.data()), code_size);

    // 3. Parse Data Section
    obj.data_section.resize(data_size);
    file.read(reinterpret_cast<char*>(obj.data_section.data()), data_size);

    // 4. Parse Symbol Table
    uint32_t num_symbols = read_int32(file);
    for (uint32_t i = 0; i < num_symbols; ++i) {
        ObjectFileSymbol sym;
        sym.name = read_string(file);
        char type_char, binding_char;
        file.read(&type_char, 1);
        file.read(&binding_char, 1);
        sym.type = static_cast<ObjectFileSymbol::Type>(type_char);
        sym.binding = static_cast<ObjectFileSymbol::Binding>(binding_char);
        sym.address = read_int32(file);
        obj.symbol_table.push_back(sym);
    }

    // 5. Parse Relocation Table
    uint32_t num_relocs = read_int32(file);
    for (uint32_t i = 0; i < num_relocs; ++i) {
        RelocationEntry reloc;
        reloc.offset = read_int32(file);
        reloc.target_symbol = read_string(file);
        obj.relocation_table.push_back(reloc);
    }

    return obj;
}