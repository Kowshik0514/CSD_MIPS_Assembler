// File: assembler.cpp
// Owner: Team
// Role: Main Assembler Driver
// Description: Main program that assembles a .stkasm file and generates:
//              1. A binary .o object file.
//              2. A human-readable .txt listing file.
//              3. A human-readable .txt hex dump of the .o file.

#include "parser.h"
#include "emitter.h"
#include "structures.h" // Need this for the functions below
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>      // For formatting hex output
#include <filesystem>   // For creating directories (C++17)
#include <sstream>      // For std::stringstream used in formatting
#include <cstdint>      // For uint8_t, uint32_t, int32_t
#include <stdexcept>    // For std::runtime_error

namespace fs = std::filesystem;

namespace {
    // --- Helper function to format bytes as a hex string ---
    std::string format_hex(const std::vector<uint8_t>& bytes, bool with_space = true) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < bytes.size(); ++i) {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
            if (with_space) ss << " ";
        }
        ss << std::dec;
        return ss.str();
    }

    // --- Helper function to read uint32 (needed for hexdump) ---
    uint32_t read_int32(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + 4 > data.size()) throw std::runtime_error("Read past end of buffer (int32)");
        uint32_t value = 0;
        value |= static_cast<uint32_t>(data[offset + 0]) << 0;
        value |= static_cast<uint32_t>(data[offset + 1]) << 8;
        value |= static_cast<uint32_t>(data[offset + 2]) << 16;
        value |= static_cast<uint32_t>(data[offset + 3]) << 24;
        offset += 4;
        return value;
    }

    // --- Helper function to write int32 (little-endian) ---
    void write_int32(std::vector<uint8_t>& out, int32_t value) {
        out.push_back(static_cast<uint8_t>((value >> 0) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }
} // anonymous namespace

// --- NEW FUNCTION: Write the annotated .txt listing file ---
void write_listing_file(const std::string& filepath, const std::string& source_filename, const AssemblyUnit& unit) {
    std::ofstream outfile(filepath);
    if (!outfile) {
        std::cerr << "Warning: Could not create listing file " << filepath << std::endl;
        return;
    }

    outfile << "--- Assembly Listing ---" << std::endl;
    outfile << "Source file: " << source_filename << std::endl;
    outfile << std::endl;

    // --- 1. Code Section Listing ---
    outfile << "--- .text section ---" << std::endl;
    outfile << "Address  | Bytes                | Source Line" << std::endl;
    outfile << "------------------------------------------------------------------" << std::endl;
    
    uint32_t current_address = 0;
    for (const auto& instr : unit.instructions) {
        RelocationEntry dummy_reloc; // Not needed for hex, just to call emit
        std::vector<uint8_t> bytes;
        try {
            bytes = instr->emit(unit, dummy_reloc);
        } catch (...) {
            bytes.push_back(0xEE); // Error byte
        }
        
        // Format: Address (hex)
        outfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << current_address << " | ";
        
        // Format: Bytes (hex)
        std::string byte_str = format_hex(bytes);
        outfile << std::left << std::setw(21) << std::setfill(' ') << byte_str << "| ";

        // Format: Source Line
        outfile << instr->source_line << std::endl;
        
        current_address += bytes.size();
    }
    outfile << std::endl;

    // --- 2. Data Section Listing ---
    outfile << "--- .data section ---" << std::endl;
    outfile << "Address  | Bytes             | Definition" << std::endl;
    outfile << "--------------------------------------------------------" << std::endl;
    current_address = 0;
    for (const auto& data : unit.data_entries) {
        outfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << current_address << " | ";
        std::vector<uint8_t> bytes;
        write_int32(bytes, data.value);
        std::string byte_str = format_hex(bytes);
        outfile << std::left << std::setw(18) << std::setfill(' ') << byte_str << "| ";
        outfile << ".static " << data.name << " " << data.value << std::endl;
        current_address += 4;
    }
    outfile << std::endl;

    // --- 3. Symbol Table Listing ---
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

// --- NEW FUNCTION: Write the annotated .txt hexdump ---
void write_hexdump_file(const std::string& filepath, const std::vector<uint8_t>& bytecode, const AssemblyUnit& unit) {
    std::ofstream outfile(filepath);
    if (!outfile) {
        std::cerr << "Warning: Could not create hexdump file " << filepath << std::endl;
        return;
    }

    // Read header info from the bytecode itself
    size_t offset = 0;
    uint32_t magic = read_int32(bytecode, offset);
    uint32_t code_size = read_int32(bytecode, offset);
    uint32_t data_size = read_int32(bytecode, offset);
    uint32_t sym_size = read_int32(bytecode, offset);
    uint32_t reloc_size = read_int32(bytecode, offset);

    outfile << "// --- Header (20 bytes) ---" << std::endl;
    outfile << format_hex(std::vector<uint8_t>(bytecode.begin(), bytecode.begin() + 4), false) << "       // Magic Number: \"STAO\" (0x" << std::hex << magic << ")" << std::endl;
    outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + 4, bytecode.begin() + 8)) << " // Code Section Size: " << std::dec << code_size << " bytes" << std::endl;
    outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + 8, bytecode.begin() + 12)) << " // Data Section Size: " << std::dec << data_size << " bytes" << std::endl;
    outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + 12, bytecode.begin() + 16)) << " // Symbol Table Size: " << std::dec << sym_size << " bytes" << std::endl;
    outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + 16, bytecode.begin() + 20)) << " // Relocation Table Size: " << std::dec << reloc_size << " bytes" << std::endl;

    // --- Code Section ---
    outfile << "\n// --- Code Section (" << std::dec << code_size << " bytes) ---" << std::endl;
    size_t code_start = 20;
    uint32_t instr_addr = 0;
    // Iterate through the parsed instructions to annotate the hexdump
    for (const auto& instr : unit.instructions) {
        RelocationEntry dummy_reloc;
        std::vector<uint8_t> bytes = instr->emit(unit, dummy_reloc); // Re-emit to get size and format
        
        // Find the matching label for this address
        std::string label_str;
        for(const auto& sym : unit.symbol_table) {
            if(sym.type == Symbol::Type::TEXT && sym.address == instr_addr && sym.is_defined) {
                // Convert int to hex string
                std::stringstream ss;
                ss << "0x" << std::hex << instr_addr;
                label_str = "// " + sym.name + ": (address " + ss.str() + ")" + "\n";
                break;
            }
        }
        if (!label_str.empty()) outfile << label_str;
        
        outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + code_start + instr_addr, bytecode.begin() + code_start + instr_addr + bytes.size()));
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
        outfile << format_hex(std::vector<uint8_t>(bytecode.begin() + data_start + data_addr, bytecode.begin() + data_start + data_addr + 4));
        outfile << " // .static " << data.name << " " << data.value << std::endl;
        data_addr += 4;
    }
    if(data_size == 0) outfile << "// (Empty)" << std::endl;


    // --- Symbol Table Section ---
    outfile << "\n// --- Symbol Table Section (" << std::dec << sym_size << " bytes) ---" << std::endl;
    outfile << "// (Raw symbol table data follows...)" << std::endl;
    size_t sym_start = 20 + code_size + data_size;
    std::string sym_hex = format_hex(std::vector<uint8_t>(bytecode.begin() + sym_start, bytecode.begin() + sym_start + sym_size));
    outfile << sym_hex << std::endl;


    // --- Relocation Table Section ---
    outfile << "\n// --- Relocation Table Section (" << std::dec << reloc_size << " bytes) ---" << std::endl;
    outfile << "// (Raw relocation table data follows...)" << std::endl;
    size_t reloc_start = 20 + code_size + data_size + sym_size;
    std::string reloc_hex = format_hex(std::vector<uint8_t>(bytecode.begin() + reloc_start, bytecode.begin() + reloc_start + reloc_size));
    outfile << reloc_hex << std::endl;
    if(reloc_size == 4) outfile << "// (No relocation entries)" << std::endl;

    outfile.close();
    std::cout << "Hexdump file successful: Generated " << filepath << std::endl;
}


int main(int argc, char* argv[]) {
    // --- UPDATED Usage ---
    // Now just takes one argument, outputs .o and .txt
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.stkasm>" << std::endl;
        std::cerr << "Example: " << argv[0] << " tests/my_program.stkasm" << std::endl;
        std::cerr << "This will create outputs in the 'outputs/' directory." << std::endl;
        return 1;
    }

    std::string input_file_path = argv[1];
    
    // --- NEW: Create output directory structure ---
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

    // Get the base filename from the input path (e.g., "array_test")
    std::string base_name = fs::path(input_file_path).stem().string();

    std::string output_o_file = o_dir + "/" + base_name + ".o";
    std::string output_listing_file = listing_dir + "/" + base_name + ".txt";
    std::string output_hexdump_file = hexdump_dir + "/" + base_name + ".txt";
    
    std::cout << "Assembling '" << input_file_path << "'..." << std::endl;

    try {
        // Step 1: Parse the input file
        AssemblyUnit unit = parse_file(input_file_path);
        std::cout << "Parsing successful: Found " 
                  << unit.instructions.size() << " instructions, "
                  << unit.symbol_table.size() << " symbols, "
                  << unit.data_entries.size() << " data entries." 
                  << std::endl;

        // Step 2: Emit object file (bytecode)
        std::vector<uint8_t> bytecode = emit_object_file(unit);
        std::cout << "Bytecode emission successful: Generated " 
                  << bytecode.size() << " bytes for " << output_o_file << "." << std::endl;
        
        // Step 3: Write the .o file
        std::ofstream outfile(output_o_file, std::ios::binary);
        if (!outfile) {
            throw std::runtime_error("Cannot open output file for writing: " + output_o_file);
        }
        outfile.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
        outfile.close();

        // --- NEW Step 4: Write the .txt listing file ---
        write_listing_file(output_listing_file, fs::path(input_file_path).filename().string(), unit);

        // --- NEW Step 5: Write the .txt hexdump file ---
        write_hexdump_file(output_hexdump_file, bytecode, unit);
        
        std::cout << "Assembly complete. Outputs are in 'outputs/' directory." << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "Assembly failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}