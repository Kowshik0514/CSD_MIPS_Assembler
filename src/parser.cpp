// File: parser.cpp
// Owner: Nischith
// Role: Parser & Front-end
// Description: Fully corrected implementation of the parser for the .stkasm language.

#include "parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Helper to trim whitespace
std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Helper to find symbols
Symbol *find_symbol_in_table(std::vector<Symbol> &table, const std::string &name) {
    auto it = std::find_if(table.begin(), table.end(),
                           [&](const Symbol &sym){ return sym.name == name; });
    return (it != table.end()) ? &(*it) : nullptr;
}

// --- NEW HELPER FUNCTION TO CALCULATE INSTRUCTION BYTE SIZE ---
// This is critical for the parser's first pass to calculate correct addresses for labels.
uint32_t get_instruction_size(const std::string& mnemonic) {
    if (mnemonic == "iconst" || mnemonic == "jmp" || mnemonic == "istore" || mnemonic == "iload") {
        return 5; // 1-byte opcode + 4-byte argument
    } else if (mnemonic == "invoke") {
        return 6; // 1-byte opcode + 4-byte address + 1-byte arg count
    } else if (mnemonic == "iadd" || mnemonic == "isub" || mnemonic == "imul" || mnemonic == "idiv" || mnemonic == "ret") {
        return 1; // 1-byte opcode
    }
    return 0; // Not a valid instruction, will be caught as an error
}

AssemblyUnit parse_file(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }

    AssemblyUnit unit;
    std::string line;
    int line_num = 0;
    enum class CurrentSection { TEXT, DATA, UNKNOWN };
    CurrentSection section = CurrentSection::UNKNOWN;
    uint32_t instruction_address = 0;
    uint32_t data_address = 0;
    std::vector<std::string> globals_to_process;

    // --- Pass 1: Identify symbols and calculate correct BYTE OFFSETS. ---
    while (std::getline(file, line)) {
        line_num++;
        std::string cleaned_line = trim(line);
        if (cleaned_line.empty() || cleaned_line[0] == '#') continue;

        if (cleaned_line[0] == '.') {
            std::stringstream ss(cleaned_line);
            std::string directive;
            ss >> directive;
            if (directive == ".text") section = CurrentSection::TEXT;
            else if (directive == ".data") section = CurrentSection::DATA;
            else if (directive == ".global") {
                std::string label_name;
                ss >> label_name;
                globals_to_process.push_back(label_name);
            } else if (directive == ".static") {
                if (section != CurrentSection::DATA) throw std::runtime_error("L" + std::to_string(line_num) + ": .static can only be used in .data section");
                std::string var_name;
                ss >> var_name;
                if (find_symbol_in_table(unit.symbol_table, var_name)) throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + var_name);
                unit.symbol_table.push_back({var_name, Symbol::Type::DATA, Symbol::Binding::LOCAL, data_address, true});
                data_address += 4;
            }
        } else if (cleaned_line.back() == ':') {
            if (section != CurrentSection::TEXT) throw std::runtime_error("L" + std::to_string(line_num) + ": Labels can only be defined in .text section");
            std::string label = cleaned_line.substr(0, cleaned_line.length() - 1);
            if (find_symbol_in_table(unit.symbol_table, label)) throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + label);
            unit.symbol_table.push_back({label, Symbol::Type::TEXT, Symbol::Binding::LOCAL, instruction_address, true});
        } else {
            if (section != CurrentSection::TEXT) throw std::runtime_error("L" + std::to_string(line_num) + ": Instructions can only be in .text section");
            
            // --- CORRECTED ADDRESS CALCULATION ---
            std::stringstream ss(cleaned_line);
            std::string mnemonic;
            ss >> mnemonic;
            uint32_t instr_size = get_instruction_size(mnemonic);
            if (instr_size == 0 && !mnemonic.empty()) { // Check for empty to avoid false error on blank lines
                 throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
            }
            instruction_address += instr_size;
        }
    }

    // Process all the .global directives
    for (const auto &name : globals_to_process) {
        Symbol *sym = find_symbol_in_table(unit.symbol_table, name);
        if (sym) {
            sym->binding = Symbol::Binding::GLOBAL;
        } else {
            unit.symbol_table.push_back({name, Symbol::Type::TEXT, Symbol::Binding::GLOBAL, 0, false});
        }
    }

    // --- Pass 2: Parse instructions and data values ---
    file.clear();
    file.seekg(0, std::ios::beg);
    line_num = 0;
    section = CurrentSection::UNKNOWN;

    while (std::getline(file, line)) {
        line_num++;
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        std::string cleaned_line = trim(line);
        if (cleaned_line.empty() || cleaned_line.back() == ':') continue;

        if (cleaned_line[0] == '.') {
            std::stringstream ss(cleaned_line);
            std::string directive;
            ss >> directive;
            if (directive == ".text") section = CurrentSection::TEXT;
            else if (directive == ".data") section = CurrentSection::DATA;
            else if (directive == ".static") {
                std::string var_name;
                int32_t value;
                ss >> var_name >> value;
                unit.data_entries.push_back({var_name, value});
            }
        } else if (section == CurrentSection::TEXT) {
            std::stringstream ss(cleaned_line);
            std::string mnemonic;
            ss >> mnemonic;

            if (mnemonic == "iconst") {
                int32_t value;
                if (!(ss >> value)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'iconst' expects an integer.");
                unit.instructions.push_back(std::make_unique<IConst>(value));
            } else if (mnemonic == "iadd") {
                unit.instructions.push_back(std::make_unique<IAdd>());
            } else if (mnemonic == "isub") {
                unit.instructions.push_back(std::make_unique<ISub>());
            } else if (mnemonic == "imul") {
                unit.instructions.push_back(std::make_unique<IMul>());
            } else if (mnemonic == "idiv") {
                unit.instructions.push_back(std::make_unique<IDiv>());
            } else if (mnemonic == "jmp") {
                std::string label;
                ss >> label;
                unit.instructions.push_back(std::make_unique<Jmp>(label));
            } else if (mnemonic == "invoke") {
                std::string label;
                int num_args;
                ss >> label >> num_args;
                unit.instructions.push_back(std::make_unique<Invoke>(label, num_args));
            } else if (mnemonic == "ret") {
                unit.instructions.push_back(std::make_unique<Ret>());
            } 
            // --- ADDED THE MISSING LOGIC FOR ISTORE and ILOAD ---
            else if (mnemonic == "istore") {
                std::string var_name;
                if (!(ss >> var_name)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'istore' expects a variable name.");
                unit.instructions.push_back(std::make_unique<IStore>(var_name));
            } else if (mnemonic == "iload") {
                std::string var_name;
                if (!(ss >> var_name)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'iload' expects a variable name.");
                unit.instructions.push_back(std::make_unique<ILoad>(var_name));
            }
            else {
                // Now this error will only trigger for truly unknown mnemonics
                if (!mnemonic.empty()) {
                    throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
                }
            }
        }
    }

    return unit;
}