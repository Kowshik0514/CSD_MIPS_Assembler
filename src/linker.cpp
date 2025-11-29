// File: linker.cpp
// Owner: Your Team
// Role: Linker Logic Implementation
// Description: Implements object file reading (from text hex dumps) and the core linking algorithm.

#include "linker.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

int32_t read_int32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) throw std::runtime_error("Read past end of buffer (int32)");
    int32_t value = 0;
    value |= static_cast<int32_t>(data[offset + 0]) << 0;
    value |= static_cast<int32_t>(data[offset + 1]) << 8;
    value |= static_cast<int32_t>(data[offset + 2]) << 16;
    value |= static_cast<int32_t>(data[offset + 3]) << 24;
    offset += 4;
    return value;
}

std::string read_string(const std::vector<uint8_t>& data, size_t& offset) {
    int32_t len = read_int32(data, offset);
    if (len < 0 || offset + len > data.size()) throw std::runtime_error("Invalid string length");
    std::string str(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return str;
}

// --- IMPROVED: Robustly parse text hex dump ---
std::vector<uint8_t> parse_hexdump_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Cannot open hex dump file: " + filepath);

    std::vector<uint8_t> buffer;
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find("//");
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        std::stringstream ss(line);
        std::string token;
        while (ss >> token) {
            // Process token. It might be "4F" or "4F415453". 
            // Iterate in chunks of 2.
            for (size_t i = 0; i + 1 < token.length(); i += 2) {
                std::string hex_pair = token.substr(i, 2);
                try {
                    if (std::isxdigit(hex_pair[0]) && std::isxdigit(hex_pair[1])) {
                        int byte_val = std::stoi(hex_pair, nullptr, 16);
                        buffer.push_back(static_cast<uint8_t>(byte_val));
                    }
                } catch (...) {
                    // Ignore invalid pairs
                }
            }
        }
    }
    return buffer;
}

ObjectFile ObjectFile::read_from(const std::string& filepath) {
    std::cout << "DEBUG: Parsing text hex dump: " << filepath << std::endl;
    std::vector<uint8_t> buffer = parse_hexdump_file(filepath);
    
    if (buffer.empty()) throw std::runtime_error("File is empty or invalid format: " + filepath);

    ObjectFile obj;
    size_t offset = 0;

    // Read header (20 bytes)
    if (buffer.size() < 20) throw std::runtime_error("Invalid object file: too small");
    uint32_t magic = read_int32(buffer, offset);
    if (magic != 0x5354414F) {
        std::cerr << "Read Magic: 0x" << std::hex << magic << std::endl;
        throw std::runtime_error("Invalid object file magic number (expected STAO): " + filepath);
    }

    uint32_t code_size = read_int32(buffer, offset);
    uint32_t data_size = read_int32(buffer, offset);
    uint32_t sym_section_size = read_int32(buffer, offset);
    uint32_t reloc_section_size = read_int32(buffer, offset);

    if (offset + code_size > buffer.size()) throw std::runtime_error("Code section size error");
    obj.code_section.assign(buffer.begin() + offset, buffer.begin() + offset + code_size);
    offset += code_size;

    if (offset + data_size > buffer.size()) throw std::runtime_error("Data section size error");
    obj.data_section.assign(buffer.begin() + offset, buffer.begin() + offset + data_size);
    offset += data_size;

    // Read Symbols
    size_t sym_start = offset;
    int32_t sym_count = read_int32(buffer, offset);
    for (int i = 0; i < sym_count; ++i) {
        Symbol sym;
        sym.name = read_string(buffer, offset);
        sym.type = static_cast<Symbol::Type>(buffer[offset++]);
        sym.binding = static_cast<Symbol::Binding>(buffer[offset++]);
        sym.is_defined = static_cast<bool>(buffer[offset++]);
        sym.address = read_int32(buffer, offset);
        obj.symbol_table.push_back(sym);
    }
    if (offset - sym_start != sym_section_size) throw std::runtime_error("Symbol table mismatch");

    // Read Relocations
    size_t reloc_start = offset;
    int32_t reloc_count = read_int32(buffer, offset);
    for (int i = 0; i < reloc_count; ++i) {
        RelocationEntry reloc;
        reloc.offset = read_int32(buffer, offset);
        reloc.target_symbol = read_string(buffer, offset);
        obj.relocation_table.push_back(reloc);
    }
    if (offset - reloc_start != reloc_section_size) throw std::runtime_error("Reloc table mismatch");

    return obj;
}

LinkedProgram link_objects(const std::vector<ObjectFile>& objects) {
    std::vector<uint8_t> final_code;
    std::vector<uint8_t> final_data;
    GlobalSymbolTable global_table;
    uint32_t code_base = 0;
    uint32_t data_base = 0;

    // Pass 1: Build Symbol Table
    for (const auto& obj : objects) {
        for (const auto& sym : obj.symbol_table) {
            if (sym.binding == Symbol::Binding::GLOBAL && sym.is_defined) {
                if (global_table.count(sym.name)) throw std::runtime_error("Duplicate: " + sym.name);
                uint32_t addr = (sym.type == Symbol::Type::TEXT) ? code_base + sym.address : data_base + sym.address;
                global_table[sym.name] = addr;
            }
        }
        code_base += obj.code_section.size();
        data_base += obj.data_section.size();
    }

    // Pass 2: Relocate & Merge
    code_base = 0;
    for (const auto& obj : objects) {
        std::vector<uint8_t> patched = obj.code_section;
        for (const auto& reloc : obj.relocation_table) {
            auto it = global_table.find(reloc.target_symbol);
            if (it == global_table.end()) throw std::runtime_error("Undefined: " + reloc.target_symbol);
            uint32_t val = it->second;
            // Patch 4 bytes
            if(reloc.offset + 4 > patched.size()) throw std::runtime_error("Reloc bounds error");
            patched[reloc.offset+0] = (val >> 0) & 0xFF;
            patched[reloc.offset+1] = (val >> 8) & 0xFF;
            patched[reloc.offset+2] = (val >> 16) & 0xFF;
            patched[reloc.offset+3] = (val >> 24) & 0xFF;
        }
        final_code.insert(final_code.end(), patched.begin(), patched.end());
        final_data.insert(final_data.end(), obj.data_section.begin(), obj.data_section.end());
        code_base += obj.code_section.size();
    }

    LinkedProgram res;
    uint32_t magic = 0x5354414B;
    if (!global_table.count("main")) throw std::runtime_error("No 'main' entry point");
    uint32_t entry = global_table["main"];
    
    // Header
    for(int i=0; i<4; i++) res.vm_bytes.push_back((magic >> (i*8)) & 0xFF);
    for(int i=0; i<4; i++) res.vm_bytes.push_back((entry >> (i*8)) & 0xFF);
    res.vm_bytes.insert(res.vm_bytes.end(), final_code.begin(), final_code.end());
    res.vm_bytes.insert(res.vm_bytes.end(), final_data.begin(), final_data.end());
    
    res.symbol_table = global_table;
    res.entry_point = entry;
    res.final_code_size = final_code.size();
    res.final_data_size = final_data.size();
    return res;
}