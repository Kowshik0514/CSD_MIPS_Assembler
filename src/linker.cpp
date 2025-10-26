// File: linker.cpp
// Role: Linker Logic Implementation
// Description: Implements object file reading and the core linking algorithm.

#include "linker.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>

// --- Helper functions for binary reading ---
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
     if (len < 0 || offset + len > data.size()) { // Basic sanity check for length
         throw std::runtime_error("Invalid string length encountered during read");
     }
    std::string str(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return str;
}

// --- Implementation of the ObjectFile reader ---
ObjectFile ObjectFile::read_from(const std::string& filepath) {
    std::cout << "DEBUG: Reading object file: " << filepath << std::endl; // <<< DEBUG
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open object file: " + filepath);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
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
    // <<< DEBUG: Print header info
    std::cout << "  DEBUG: Header - Code Size: " << code_size << ", Data Size: " << data_size
              << ", Sym Size: " << sym_section_size << ", Reloc Size: " << reloc_section_size << std::endl;


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
        sym.name = read_string(buffer, offset);                 // 1. Name
        sym.type = static_cast<Symbol::Type>(buffer[offset++]); // 2. Type
        sym.binding = static_cast<Symbol::Binding>(buffer[offset++]); // 3. Binding
        sym.is_defined = static_cast<bool>(buffer[offset++]);   // 4. is_defined FLAG <-- Read it here!
        sym.address = read_int32(buffer, offset);               // 5. Address
        obj.symbol_table.push_back(sym);
        // Debug print (already present in your code)
        std::cout << "    DEBUG: Symbol Read [" << i << "]: Name='" << sym.name
                  << "', Type=" << (sym.type == Symbol::Type::TEXT ? "TEXT" : "DATA")
                  << ", Binding=" << (sym.binding == Symbol::Binding::GLOBAL ? "GLOBAL" : "LOCAL")
                  << ", Defined=" << (sym.is_defined ? "true" : "false") // Now this will be correct!
                  << ", Address=0x" << std::hex << sym.address << std::dec << std::endl;
    }
    if (offset - sym_table_start_offset != sym_section_size) {
         std::cerr << "WARNING: Symbol table size mismatch in file: " << filepath
                   << ". Expected " << sym_section_size << ", Read " << (offset - sym_table_start_offset) << std::endl;
         // Allow continuing for debugging, but this indicates a problem in the emitter or reader
         offset = sym_table_start_offset + sym_section_size; // Force offset correction
    }


    // Read Relocation Table
    size_t reloc_table_start_offset = offset;
    int32_t reloc_count = read_int32(buffer, offset);
    obj.relocation_table.reserve(reloc_count);
    std::cout << "  DEBUG: Reading Relocation Table (" << reloc_count << " entries)..." << std::endl; // <<< DEBUG
    for (int i = 0; i < reloc_count; ++i) {
        RelocationEntry reloc;
        reloc.offset = read_int32(buffer, offset);
        reloc.target_symbol = read_string(buffer, offset);
        obj.relocation_table.push_back(reloc);
         // <<< DEBUG: Print each relocation read
        std::cout << "    DEBUG: Reloc Read [" << i << "]: Offset=" << reloc.offset
                  << ", Target='" << reloc.target_symbol << "'" << std::endl;
    }
    if (offset - reloc_table_start_offset != reloc_section_size) {
        std::cerr << "WARNING: Relocation table size mismatch in file: " << filepath
                  << ". Expected " << reloc_section_size << ", Read " << (offset - reloc_table_start_offset) << std::endl;
        // Allow continuing for debugging
         offset = reloc_table_start_offset + reloc_section_size; // Force offset correction
    }
    std::cout << "  DEBUG: Finished reading " << filepath << std::endl; // <<< DEBUG
    return obj;
}


// --- Main Linker Logic ---
std::vector<uint8_t> link_objects(const std::vector<ObjectFile>& objects) {
    std::cout << "DEBUG: Starting Linker Pass 1: Building Global Symbol Table..." << std::endl; // <<< DEBUG
    std::vector<uint8_t> final_code_section;
    std::vector<uint8_t> final_data_section;
    GlobalSymbolTable global_symbol_table;

    uint32_t current_code_base_offset = 0;
    uint32_t current_data_base_offset = 0;

    // 1. First Pass: Calculate offsets and build the global symbol table
    int obj_index = 0;
    for (const auto& obj : objects) {
        std::cout << "  DEBUG: Pass 1 - Processing Object File #" << obj_index++ << std::endl; // <<< DEBUG
        // Build global symbol table for DEFINED global symbols
        for (const auto& sym : obj.symbol_table) {
            // <<< DEBUG: Print symbol being considered in Pass 1
            std::cout << "    DEBUG: Pass 1 - Considering Symbol: Name='" << sym.name
                      << "', Binding=" << (sym.binding == Symbol::Binding::GLOBAL ? "GLOBAL" : "LOCAL")
                      << ", Defined=" << (sym.is_defined ? "true" : "false") << std::endl;

            // Only add defined global symbols to the master table
            if (sym.binding == Symbol::Binding::GLOBAL && sym.is_defined) {
                if (global_symbol_table.count(sym.name)) {
                    // This is a critical error - symbol defined in multiple files
                     std::cerr << "      DEBUG: ERROR! Duplicate global definition found for '" << sym.name << "'" << std::endl; // <<< DEBUG
                    throw std::runtime_error("Duplicate global symbol definition: " + sym.name);
                }
                // Calculate the final absolute address
                uint32_t final_address = (sym.type == Symbol::Type::TEXT)
                    ? current_code_base_offset + sym.address
                    : current_data_base_offset + sym.address;
                global_symbol_table[sym.name] = final_address;
                // <<< DEBUG: Print symbol added to global table
                std::cout << "      DEBUG: Added GLOBAL symbol '" << sym.name << "' to GST with final address 0x"
                          << std::hex << final_address << std::dec << std::endl;
            }
        }
        // Keep track of base offsets for the next file
        current_code_base_offset += obj.code_section.size();
        current_data_base_offset += obj.data_section.size();
    }
    std::cout << "DEBUG: Linker Pass 1 Complete. Global Symbol Table built." << std::endl; // <<< DEBUG

    // 2. Second Pass: Perform relocation and merge sections
    std::cout << "DEBUG: Starting Linker Pass 2: Relocation and Merging..." << std::endl; // <<< DEBUG
    current_code_base_offset = 0; // Reset for calculating relative offsets during patching
    obj_index = 0;
    for (const auto& obj : objects) {
         std::cout << "  DEBUG: Pass 2 - Processing Object File #" << obj_index++ << std::endl; // <<< DEBUG
        std::vector<uint8_t> patched_code = obj.code_section; // Make a copy to modify

        // Apply relocations for this object file
        for (const auto& reloc : obj.relocation_table) {
             std::cout << "    DEBUG: Pass 2 - Applying Reloc: Offset=" << reloc.offset << ", Target='" << reloc.target_symbol << "'" << std::endl; // <<< DEBUG
            // Find the target symbol in the global table
            auto it = global_symbol_table.find(reloc.target_symbol);
            if (it == global_symbol_table.end()) {
                // This is a critical error - symbol used but never defined globally
                std::cerr << "      DEBUG: ERROR! Undefined global symbol '" << reloc.target_symbol << "' referenced." << std::endl; // <<< DEBUG
                throw std::runtime_error("Undefined global symbol referenced during relocation: " + reloc.target_symbol);
            }
            uint32_t final_address = it->second;
            std::cout << "      DEBUG: Found Target Address = 0x" << std::hex << final_address << std::dec << std::endl; // <<< DEBUG


            // Patch the 4-byte address into the code section copy
            if (reloc.offset + 4 > patched_code.size()) {
                std::cerr << "      DEBUG: ERROR! Relocation offset " << reloc.offset << " is out of bounds (Code Size: " << patched_code.size() << ")" << std::endl; // <<< DEBUG
                throw std::runtime_error("Relocation offset out of bounds for symbol: " + reloc.target_symbol);
            }
            // Write the address in little-endian format
            patched_code[reloc.offset + 0] = (final_address >> 0)  & 0xFF;
            patched_code[reloc.offset + 1] = (final_address >> 8)  & 0xFF;
            patched_code[reloc.offset + 2] = (final_address >> 16) & 0xFF;
            patched_code[reloc.offset + 3] = (final_address >> 24) & 0xFF;
             std::cout << "      DEBUG: Patched address at offset " << reloc.offset << std::endl; // <<< DEBUG
        }

        // Add the now-patched code to the final binary
        final_code_section.insert(final_code_section.end(), patched_code.begin(), patched_code.end());
        // Append this object's data section
        final_data_section.insert(final_data_section.end(), obj.data_section.begin(), obj.data_section.end());

        current_code_base_offset += obj.code_section.size();
    }
     std::cout << "DEBUG: Linker Pass 2 Complete. Sections merged and relocated." << std::endl; // <<< DEBUG

    // 3. Create the final .vm executable file content
     std::cout << "DEBUG: Creating final executable header..." << std::endl; // <<< DEBUG
    std::vector<uint8_t> final_executable;
    uint32_t magic_number = 0x5354414B; // "STAK" (VM executable magic number)

    // Find the entry point ('main' function) address
    if (!global_symbol_table.count("main")) {
        throw std::runtime_error("Entry point 'main' not found in global symbols.");
    }
    uint32_t entry_point = global_symbol_table.at("main");
     std::cout << "  DEBUG: Entry point 'main' found at address 0x" << std::hex << entry_point << std::dec << std::endl; // <<< DEBUG


    // Write header for .vm file (Magic Number + Entry Point Address)
    for (int i = 0; i < 4; i++) final_executable.push_back((magic_number >> (i * 8)) & 0xFF);
    for (int i = 0; i < 4; i++) final_executable.push_back((entry_point >> (i * 8)) & 0xFF);

    // Append the final merged and patched code section
    final_executable.insert(final_executable.end(), final_code_section.begin(), final_code_section.end());

    // Append the final merged data section
    final_executable.insert(final_executable.end(), final_data_section.begin(), final_data_section.end());

     std::cout << "DEBUG: Final executable size: " << final_executable.size() << " bytes." << std::endl; // <<< DEBUG
    return final_executable;
}