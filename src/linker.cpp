// File: linker.cpp
// Owner: Your Team (e.g., Rashmitha, Kowshik)
// Role: Linker Logic Implementation
// Description: Implements object file reading and the core linking algorithm.

#include "linker.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

// --- Helper functions for binary reading ---
// (These must be identical to the helpers in your assembler's emitter.cpp)
int32_t read_int32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) throw std::runtime_error("Read past end of buffer (int32)");
    int32_t value = 0;
    // Little-endian read
    value |= static_cast<int32_t>(data[offset + 0]) << 0;
    value |= static_cast<int32_t>(data[offset + 1]) << 8;
    value |= static_cast<int32_t>(data[offset + 2]) << 16;
    value |= static_cast<int32_t>(data[offset + 3]) << 24;
    offset += 4;
    return value;
}

std::string read_string(const std::vector<uint8_t>& data, size_t& offset) {
    int32_t len = read_int32(data, offset);
     if (len < 0 || offset + len > data.size()) {
         throw std::runtime_error("Invalid string length encountered during read");
     }
    std::string str(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return str;
}

// --- Implementation of the ObjectFile reader ---
// This parses the .o file format your assembler creates
ObjectFile ObjectFile::read_from(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate); // Open at end to get size
    if (!file) throw std::runtime_error("Cannot open object file: " + filepath);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg); // Go back to start
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read object file: " + filepath);
    }
    file.close();

    ObjectFile obj;
    size_t offset = 0;

    // Read header (20 bytes)
    uint32_t magic = read_int32(buffer, offset);
    if (magic != 0x5354414F) throw std::runtime_error("Invalid object file magic number (expected STAO): " + filepath);

    uint32_t code_size = read_int32(buffer, offset);
    uint32_t data_size = read_int32(buffer, offset);
    uint32_t sym_section_size = read_int32(buffer, offset);
    uint32_t reloc_section_size = read_int32(buffer, offset);

    // Read sections into the ObjectFile struct
    if (offset + code_size > buffer.size()) throw std::runtime_error("Code section size exceeds buffer in: " + filepath);
    obj.code_section.assign(buffer.begin() + offset, buffer.begin() + offset + code_size);
    offset += code_size;

    if (offset + data_size > buffer.size()) throw std::runtime_error("Data section size exceeds buffer in: " + filepath);
    obj.data_section.assign(buffer.begin() + offset, buffer.begin() + offset + data_size);
    offset += data_size;

    // Read Symbol Table
    size_t sym_table_start_offset = offset;
    int32_t sym_count = read_int32(buffer, offset);
    obj.symbol_table.reserve(sym_count);
    for (int i = 0; i < sym_count; ++i) {
        Symbol sym;
        sym.name = read_string(buffer, offset);
        sym.type = static_cast<Symbol::Type>(buffer[offset++]);
        sym.binding = static_cast<Symbol::Binding>(buffer[offset++]);
        sym.is_defined = static_cast<bool>(buffer[offset++]); // Read the flag
        sym.address = read_int32(buffer, offset);
        obj.symbol_table.push_back(sym);
    }
    if (offset - sym_table_start_offset != sym_section_size) {
         throw std::runtime_error("Symbol table size mismatch in file: " + filepath);
    }

    // Read Relocation Table
    size_t reloc_table_start_offset = offset;
    int32_t reloc_count = read_int32(buffer, offset);
    obj.relocation_table.reserve(reloc_count);
    for (int i = 0; i < reloc_count; ++i) {
        RelocationEntry reloc;
        reloc.offset = read_int32(buffer, offset);
        reloc.target_symbol = read_string(buffer, offset);
        obj.relocation_table.push_back(reloc);
    }
    if (offset - reloc_table_start_offset != reloc_section_size) {
         throw std::runtime_error("Relocation table size mismatch in file: " + filepath);
    }

    return obj;
}


// --- Main Linker Logic ---
LinkedProgram link_objects(const std::vector<ObjectFile>& objects) {
    std::vector<uint8_t> final_code_section;
    std::vector<uint8_t> final_data_section;
    GlobalSymbolTable global_symbol_table;

    uint32_t current_code_base_offset = 0;
    uint32_t current_data_base_offset = 0;

    // 1. First Pass: Calculate offsets and build the global symbol table
    for (const auto& obj : objects) {
        // Build global symbol table for DEFINED global symbols
        for (const auto& sym : obj.symbol_table) {
            // Only add defined global symbols to the master table
            if (sym.binding == Symbol::Binding::GLOBAL && sym.is_defined) {
                if (global_symbol_table.count(sym.name)) {
                    throw std::runtime_error("Duplicate global symbol definition: " + sym.name);
                }
                // Calculate the final absolute address
                uint32_t final_address = (sym.type == Symbol::Type::TEXT)
                    ? current_code_base_offset + sym.address
                    : current_data_base_offset + sym.address;
                global_symbol_table[sym.name] = final_address;
            }
        }
        // Keep track of base offsets for the next file
        current_code_base_offset += obj.code_section.size();
        current_data_base_offset += obj.data_section.size();
    }

    // 2. Second Pass: Perform relocation and merge sections
    current_code_base_offset = 0; // Reset
    for (const auto& obj : objects) {
        std::vector<uint8_t> patched_code = obj.code_section; // Make a copy to modify

        // Apply relocations for this object file
        for (const auto& reloc : obj.relocation_table) {
            // Find the target symbol in the global table
            auto it = global_symbol_table.find(reloc.target_symbol);
            if (it == global_symbol_table.end()) {
                throw std::runtime_error("Undefined global symbol referenced: " + reloc.target_symbol);
            }
            uint32_t final_address = it->second;

            // Patch the 4-byte address into the code section copy
            if (reloc.offset + 4 > patched_code.size()) {
                throw std::runtime_error("Relocation offset out of bounds for symbol: " + reloc.target_symbol);
            }
            // Write the address in little-endian format
            patched_code[reloc.offset + 0] = (final_address >> 0)  & 0xFF;
            patched_code[reloc.offset + 1] = (final_address >> 8)  & 0xFF;
            patched_code[reloc.offset + 2] = (final_address >> 16) & 0xFF;
            patched_code[reloc.offset + 3] = (final_address >> 24) & 0xFF;
        }

        // Add the now-patched code to the final binary
        final_code_section.insert(final_code_section.end(), patched_code.begin(), patched_code.end());
        // Append this object's data section
        final_data_section.insert(final_data_section.end(), obj.data_section.begin(), obj.data_section.end());

        current_code_base_offset += obj.code_section.size();
    }

    // 3. Create the final .vm executable file content
    std::vector<uint8_t> final_executable_bytes;
    uint32_t magic_number = 0x5354414B; // "STAK" (VM executable magic number)

    if (!global_symbol_table.count("main")) {
        throw std::runtime_error("Entry point 'main' not found.");
    }
    uint32_t entry_point = global_symbol_table.at("main");

    // Write header for .vm file (Magic Number + Entry Point Address)
    for (int i = 0; i < 4; i++) final_executable_bytes.push_back((magic_number >> (i * 8)) & 0xFF);
    for (int i = 0; i < 4; i++) final_executable_bytes.push_back((entry_point >> (i * 8)) & 0xFF);

    // Append the final merged and patched code section
    final_executable_bytes.insert(final_executable_bytes.end(), final_code_section.begin(), final_code_section.end());

    // Append the final merged data section
    final_executable_bytes.insert(final_executable_bytes.end(), final_data_section.begin(), final_data_section.end());

    // 4. Create the LinkedProgram struct to return
    LinkedProgram result;
    result.vm_bytes = final_executable_bytes;
    result.symbol_table = global_symbol_table;
    result.entry_point = entry_point;
    result.final_code_size = final_code_section.size();
    result.final_data_size = final_data_section.size();

    return result;
}