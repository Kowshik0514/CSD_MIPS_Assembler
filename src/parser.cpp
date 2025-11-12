// File: parser.cpp
// Owner: Nischith
// Role: Parser & Front-end
// Description: Fully updated 2-pass parser for all new instructions.

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

// --- Get size of an instruction from its mnemonic ---
uint32_t get_instruction_size(const std::string& mnemonic) {
    if (mnemonic == "iconst" || mnemonic == "jmp" || mnemonic == "istore" || 
        mnemonic == "iload" || mnemonic == "jmp_if_false") {
        return 5; // 1-byte opcode + 4-byte argument
    }
    if (mnemonic == "invoke") {
        return 6; // 1-byte opcode + 4-byte address + 1-byte arg count
    }
    if (mnemonic == "iadd" || mnemonic == "isub" || mnemonic == "imul" || mnemonic == "idiv" || 
        mnemonic == "ret" || mnemonic == "NEW_ARRAY" || mnemonic == "SET_ELEM" || 
        mnemonic == "GET_ELEM" || mnemonic == "icmp_eq" || mnemonic == "icmp_lt" || 
        mnemonic == "icmp_gt" || mnemonic == "PRINT_I" || 
        // --- ADD NEW STRING/IO OPCODES ---
        mnemonic == "NEW_STRING" || mnemonic == "SET_CHAR" || 
        mnemonic == "GET_CHAR" || mnemonic == "PRINT_S") {
        return 1; // 1-byte opcode
    }
    return 0; // Not a valid instruction
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
    std::vector<std::string> source_lines; // Store all original lines
    
    while (std::getline(file, line)) {
        source_lines.push_back(line);
    }
    file.close();
    
    line_num = 0;

    // --- Pass 1: Find symbols and calculate byte offsets. ---
    for (const auto& original_line : source_lines) {
        line_num++;
        std::string cleaned_line = trim(original_line.substr(0, original_line.find('#')));
        
        if (cleaned_line.empty()) continue;

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
                if (section != CurrentSection::DATA) throw std::runtime_error("L" + std::to_string(line_num) + ": .static in wrong section");
                std::string var_name;
                ss >> var_name;
                if (find_symbol_in_table(unit.symbol_table, var_name)) throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + var_name);
                unit.symbol_table.push_back({var_name, Symbol::Type::DATA, Symbol::Binding::LOCAL, data_address, true}); // is_defined = true
                data_address += 4;
            }
        } else if (cleaned_line.back() == ':') {
            if (section != CurrentSection::TEXT) throw std::runtime_error("L" + std::to_string(line_num) + ": Label in wrong section");
            std::string label = cleaned_line.substr(0, cleaned_line.length() - 1);
            if (find_symbol_in_table(unit.symbol_table, label)) throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + label);
            unit.symbol_table.push_back({label, Symbol::Type::TEXT, Symbol::Binding::LOCAL, instruction_address, true}); // is_defined = true
        } else {
            if (section != CurrentSection::TEXT) throw std::runtime_error("L" + std::to_string(line_num) + ": Instruction in wrong section");
            
            std::stringstream ss(cleaned_line);
            std::string mnemonic;
            ss >> mnemonic;
            uint32_t instr_size = get_instruction_size(mnemonic);
            if (instr_size == 0) throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
            instruction_address += instr_size;
        }
    }

    // After Pass 1, update GLOBAL flags
    for (const auto &name : globals_to_process) {
        Symbol *sym = find_symbol_in_table(unit.symbol_table, name);
        if (sym) {
            sym->binding = Symbol::Binding::GLOBAL;
        } else {
            unit.symbol_table.push_back({name, Symbol::Type::TEXT, Symbol::Binding::GLOBAL, 0, false}); // External symbol, is_defined = false
        }
    }

    // --- Pass 2: Parse instructions ---
    line_num = 0;
    section = CurrentSection::UNKNOWN;

    for (const auto& original_line : source_lines) {
        line_num++;
        std::string cleaned_line = trim(original_line.substr(0, original_line.find('#')));
        
        if (cleaned_line.empty() || cleaned_line.back() == ':' || cleaned_line[0] == '#') continue;

        if (cleaned_line[0] == '.') {
            if (cleaned_line.rfind(".text", 0) == 0) section = CurrentSection::TEXT;
            else if (cleaned_line.rfind(".data", 0) == 0) section = CurrentSection::DATA;
            else if (cleaned_line.rfind(".static", 0) == 0) {
                std::stringstream ss(cleaned_line);
                std::string directive, var_name;
                int32_t value;
                ss >> directive >> var_name >> value;
                unit.data_entries.push_back({var_name, value});
            }
            continue;
        } 
        
        if (section == CurrentSection::TEXT) {
            std::stringstream ss(cleaned_line);
            std::string mnemonic;
            ss >> mnemonic;
            std::unique_ptr<Instruction> instr;

            if (mnemonic == "iconst") {
                int32_t value;
                if (!(ss >> value)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'iconst' expects an integer.");
                instr = std::make_unique<IConst>(value);
            } else if (mnemonic == "iadd") {
                instr = std::make_unique<IAdd>();
            } else if (mnemonic == "isub") {
                instr = std::make_unique<ISub>();
            } else if (mnemonic == "imul") {
                instr = std::make_unique<IMul>();
            } else if (mnemonic == "idiv") {
                instr = std::make_unique<IDiv>();
            } else if (mnemonic == "ret") {
                instr = std::make_unique<Ret>();
            } else if (mnemonic == "jmp") {
                std::string label;
                ss >> label;
                instr = std::make_unique<Jmp>(label);
            } else if (mnemonic == "invoke") {
                std::string label;
                int num_args;
                ss >> label >> num_args;
                instr = std::make_unique<Invoke>(label, (uint8_t)num_args);
            } 
            else if (mnemonic == "istore") {
                int32_t index;
                if (!(ss >> index)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'istore' expects an integer index.");
                instr = std::make_unique<IStore>(index);
            } else if (mnemonic == "iload") {
                int32_t index;
                if (!(ss >> index)) throw std::runtime_error("L" + std::to_string(line_num) + ": 'iload' expects an integer index.");
                instr = std::make_unique<ILoad>(index);
            }
            else if (mnemonic == "NEW_ARRAY") {
                instr = std::make_unique<NewArray>();
            } else if (mnemonic == "SET_ELEM") {
                instr = std::make_unique<SetElem>();
            } else if (mnemonic == "GET_ELEM") {
                instr = std::make_unique<GetElem>();
            }
            else if (mnemonic == "NEW_STRING") {
                instr = std::make_unique<NewString>();
            } else if (mnemonic == "SET_CHAR") {
                instr = std::make_unique<SetChar>();
            } else if (mnemonic == "GET_CHAR") {
                instr = std::make_unique<GetChar>();
            }
            else if (mnemonic == "icmp_eq") {
                instr = std::make_unique<ICmpEQ>();
            } else if (mnemonic == "icmp_lt") {
                instr = std::make_unique<ICmpLT>();
            } else if (mnemonic == "icmp_gt") {
                instr = std::make_unique<ICmpGT>();
            } else if (mnemonic == "jmp_if_false") {
                std::string label;
                ss >> label;
                instr = std::make_unique<JmpIfFalse>(label);
            }
            else if (mnemonic == "PRINT_I") {
                instr = std::make_unique<PrintI>();
            }
            else if (mnemonic == "PRINT_S") {
                instr = std::make_unique<PrintS>();
            }
            else {
                throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
            }
            
            instr->source_line = trim(original_line);
            unit.instructions.push_back(std::move(instr));
        }
    }

    return unit;
}