// File: assembler.cpp
// Owner: Team
// Role: Main Assembler Driver
// Description: Main program that assembles a .stkasm file and generates outputs.

#include "parser.h"
#include "emitter.h"
#include "structures.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

// --- Helper functions for binary reading (needed for hexdump generation) ---
// Note: These must match the endianness logic in emitter.cpp/linker.cpp
int32_t read_int32_le(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0; // Should throw, but soft fail for dump
    int32_t value = 0;
    value |= static_cast<int32_t>(data[offset + 0]) << 0;
    value |= static_cast<int32_t>(data[offset + 1]) << 8;
    value |= static_cast<int32_t>(data[offset + 2]) << 16;
    value |= static_cast<int32_t>(data[offset + 3]) << 24;
    offset += 4;
    return value;
}

// --- Helper function to format bytes as a hex string ---
std::string format_hex(const std::vector<uint8_t>& bytes, bool with_space = true) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < bytes.size(); ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
        if (with_space) ss << " ";
    }
    return ss.str();
}

// --- Write the annotated .txt listing file ---
void write_listing_file(const std::string& filepath, const std::string& source_filename, const AssemblyUnit& unit) {
    std::ofstream outfile(filepath);
    if (!outfile) {
        std::cerr << "Warning: Could not create listing file " << filepath << std::endl;
        return;
    }

    outfile << "--- Assembly Listing ---" << std::endl;
    outfile << "Source file: " << source_filename << std::endl;
    outfile << std::endl;

    outfile << "--- .text section ---" << std::endl;
    outfile << "Address  | Bytes                | Source Line" << std::endl;
    outfile << "------------------------------------------------------------------" << std::endl;
    
    uint32_t current_address = 0;
    for (const auto& instr : unit.instructions) {
        RelocationEntry dummy_reloc;
        std::vector<uint8_t> bytes;
        try {
            bytes = instr->emit(unit, dummy_reloc);
        } catch (...) {
            bytes.push_back(0xEE);
        }
        
        outfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << current_address << " | ";
        std::string byte_str = format_hex(bytes);
        outfile << std::left << std::setw(21) << std::setfill(' ') << byte_str << "| ";
        outfile << instr->source_line << std::endl;
        
        current_address += bytes.size();
    }
    outfile << std::endl;

    outfile << "--- .data section ---" << std::endl;
    outfile << "Address  | Bytes             | Definition" << std::endl;
    outfile << "--------------------------------------------------------" << std::endl;
    current_address = 0;
    for (const auto& data : unit.data_entries) {
        outfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << current_address << " | ";
        std::vector<uint8_t> bytes;
        // Write manually to vector to use format_hex
        bytes.push_back((data.value >> 0) & 0xFF);
        bytes.push_back((data.value >> 8) & 0xFF);
        bytes.push_back((data.value >> 16) & 0xFF);
        bytes.push_back((data.value >> 24) & 0xFF);
        
        std::string byte_str = format_hex(bytes);
        outfile << std::left << std::setw(18) << std::setfill(' ') << byte_str << "| ";
        outfile << ".static " << data.name << " " << data.value << std::endl;
        current_address += 4;
    }
    outfile << std::endl;

    outfile << "--- Symbol Table ---" << std::endl;
    outfile << "Name           | Type | Binding | Def | Address" << std::endl;
    outfile << "--------------------------------------------------------" << std::endl;
    for (const auto& sym : unit.symbol_table) {
        outfile << std::left << std::setw(15) << std::setfill(' ') << sym.name << "| ";
        outfile << std::left << std::setw(5) << (sym.type == Symbol::Type::TEXT ? "TEXT" : "DATA") << "| ";
        outfile << std::left << std::setw(8) << (sym.binding == Symbol::Binding::GLOBAL ? "GLOBAL" : "LOCAL") << "| ";
        outfile << std::left << std::setw(4) << (sym.is_defined ? "yes" : "no") << "| ";
        outfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << sym.address << std::dec << std::endl;
    }
    outfile.close();
    std::cout << "Listing file successful: Generated " << filepath << std::endl;
}

// --- Write the annotated .txt hexdump file ---
void write_hexdump_file(const std::string& filepath, const std::vector<uint8_t>& bytecode, const AssemblyUnit& unit) {
    std::ofstream outfile(filepath);
    if (!outfile) {
        std::cerr << "Warning: Could not create hexdump file " << filepath << std::endl;
        return;
    }

    size_t offset = 0;
    // Helper lambda to safely get sub-vector
    auto get_bytes = [&](size_t start, size_t len) {
        if (start + len > bytecode.size()) return std::vector<uint8_t>();
        return std::vector<uint8_t>(bytecode.begin() + start, bytecode.begin() + start + len);
    };

    uint32_t magic = read_int32_le(bytecode, offset);
    uint32_t code_size = read_int32_le(bytecode, offset);
    uint32_t data_size = read_int32_le(bytecode, offset);
    uint32_t sym_size = read_int32_le(bytecode, offset);
    uint32_t reloc_size = read_int32_le(bytecode, offset);

    outfile << "// --- Header (20 bytes) ---" << std::endl;
    // FIX: Changed 'false' to 'true' to ensure spaces between bytes (4F 41 54 53)
    outfile << format_hex(get_bytes(0, 4), true) << "      // Magic Number: \"STAO\" (0x" << std::hex << magic << ")" << std::endl;
    outfile << format_hex(get_bytes(4, 4)) << " // Code Section Size: " << std::dec << code_size << " bytes" << std::endl;
    outfile << format_hex(get_bytes(8, 4)) << " // Data Section Size: " << std::dec << data_size << " bytes" << std::endl;
    outfile << format_hex(get_bytes(12, 4)) << " // Symbol Table Size: " << std::dec << sym_size << " bytes" << std::endl;
    outfile << format_hex(get_bytes(16, 4)) << " // Relocation Table Size: " << std::dec << reloc_size << " bytes" << std::endl;

    // --- Code Section ---
    outfile << "\n// --- Code Section (" << std::dec << code_size << " bytes) ---" << std::endl;
    size_t code_start = 20;
    uint32_t instr_addr = 0;
    for (const auto& instr : unit.instructions) {
        RelocationEntry dummy_reloc;
        std::vector<uint8_t> bytes = instr->emit(unit, dummy_reloc);
        
        std::string label_str;
        for(const auto& sym : unit.symbol_table) {
            if(sym.type == Symbol::Type::TEXT && sym.address == instr_addr && sym.is_defined) {
                std::stringstream ss;
                ss << "0x" << std::hex << instr_addr;
                label_str = "// " + sym.name + ": (address " + ss.str() + ")" + "\n";
                break;
            }
        }
        if (!label_str.empty()) outfile << label_str;
        
        outfile << format_hex(get_bytes(code_start + instr_addr, bytes.size()));
        outfile << " // " << instr->source_line << std::endl;
        instr_addr += bytes.size();
    }

    // --- Data Section ---
    outfile << "\n// --- Data Section (" << std::dec << data_size << " bytes) ---" << std::endl;
    size_t data_start = 20 + code_size;
    uint32_t data_addr = 0;
    for (const auto& data : unit.data_entries) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << data_addr;
        outfile << ss.str() << ": ";
        outfile << format_hex(get_bytes(data_start + data_addr, 4));
        outfile << " // .static " << data.name << " " << data.value << std::endl;
        data_addr += 4;
    }
    if(data_size == 0) outfile << "// (Empty)" << std::endl;

    // --- Symbol Table ---
    outfile << "\n// --- Symbol Table Section (" << std::dec << sym_size << " bytes) ---" << std::endl;
    size_t sym_start = 20 + code_size + data_size;
    outfile << format_hex(get_bytes(sym_start, sym_size)) << std::endl;

    // --- Relocations ---
    outfile << "\n// --- Relocation Table Section (" << std::dec << reloc_size << " bytes) ---" << std::endl;
    size_t reloc_start = 20 + code_size + data_size + sym_size;
    outfile << format_hex(get_bytes(reloc_start, reloc_size)) << std::endl;
    if(reloc_size == 0) outfile << "// (No relocation entries)" << std::endl;

    outfile.close();
    std::cout << "Hexdump file successful: Generated " << filepath << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.stkasm>" << std::endl;
        return 1;
    }

    std::string input_file_path = argv[1];
    std::string base_output_dir = "outputs";
    std::string o_dir = base_output_dir + "/o";
    std::string listing_dir = base_output_dir + "/listing";
    std::string hexdump_dir = base_output_dir + "/hexdump";
    
    try {
        fs::create_directories(o_dir);
        fs::create_directories(listing_dir);
        fs::create_directories(hexdump_dir);
    } catch (const std::exception& e) {
        std::cerr << "Error creating output directories: " << e.what() << std::endl;
        return 1;
    }

    std::string base_name = fs::path(input_file_path).stem().string();
    std::string output_o_file = o_dir + "/" + base_name + ".o";
    std::string output_listing_file = listing_dir + "/" + base_name + ".txt";
    std::string output_hexdump_file = hexdump_dir + "/" + base_name + ".txt";
    
    std::cout << "Assembling '" << input_file_path << "'..." << std::endl;

    try {
        AssemblyUnit unit = parse_file(input_file_path);
        std::vector<uint8_t> bytecode = emit_object_file(unit);
        
        std::ofstream outfile(output_o_file, std::ios::binary);
        if (!outfile) throw std::runtime_error("Cannot open output file: " + output_o_file);
        outfile.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
        outfile.close();

        write_listing_file(output_listing_file, fs::path(input_file_path).filename().string(), unit);
        write_hexdump_file(output_hexdump_file, bytecode, unit);
        
        std::cout << "✅ Assembly complete. Outputs in 'outputs/'." << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "❌ Assembly failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}