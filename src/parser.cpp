// File: parser.cpp
// Owner: Nischith
// Role: Parser & Front-end
// Description: Fully updated parser with SMART COMMENT handling for #Labels.

#include "parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// --- SMART COMMENT STRIPPER ---
// Treats '#' as a comment only if followed by a space or end-of-line.
// Allows labels like '#L0' to pass through.
std::string strip_comments(const std::string &line) {
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == '#') {
            // Check next char
            if (i + 1 == line.length() || std::isspace(line[i + 1])) {
                // It's a comment (e.g. "# Comment" or "#" at end)
                return line.substr(0, i);
            }
            // Otherwise, it's a label like "#L0", keep going.
        }
    }
    return line;
}

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

Symbol *find_symbol_in_table(std::vector<Symbol> &table, const std::string &name) {
    auto it = std::find_if(table.begin(), table.end(),
                           [&](const Symbol &sym){ return sym.name == name; });
    return (it != table.end()) ? &(*it) : nullptr;
}

uint32_t get_instruction_size(const std::string& mnemonic) {
    if (mnemonic == "iconst" || mnemonic == "jmp" || mnemonic == "istore" || 
        mnemonic == "iload" || mnemonic == "jmp_if_false" || 
        mnemonic == "jnz") {
        return 5;
    }
    if (mnemonic == "invoke") {
        return 6;
    }
    if (mnemonic == "iadd" || mnemonic == "isub" || mnemonic == "imul" || mnemonic == "idiv" || 
        mnemonic == "ret" || mnemonic == "NEW_ARRAY" || mnemonic == "SET_ELEM" || 
        mnemonic == "GET_ELEM" || mnemonic == "icmp_eq" || mnemonic == "icmp_lt" || 
        mnemonic == "icmp_gt" || mnemonic == "PRINT_I" || 
        mnemonic == "NEW_STRING" || mnemonic == "SET_CHAR" || 
        mnemonic == "GET_CHAR" || mnemonic == "PRINT_S") {
        return 1;
    }
    return 0;
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
    std::vector<std::string> source_lines;
    
    while (std::getline(file, line)) {
        source_lines.push_back(line);
    }
    file.close();
    
    // --- Pass 1: Find symbols ---
    line_num = 0;
    for (const auto& original_line : source_lines) {
        line_num++;
        std::string cleaned_line = trim(strip_comments(original_line));
        
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
                unit.symbol_table.push_back({var_name, Symbol::Type::DATA, Symbol::Binding::LOCAL, data_address, true});
                data_address += 4;
            }
        } else if (cleaned_line.back() == ':') {
            if (section != CurrentSection::TEXT) throw std::runtime_error("L" + std::to_string(line_num) + ": Label in wrong section");
            std::string label = cleaned_line.substr(0, cleaned_line.length() - 1);
            if (find_symbol_in_table(unit.symbol_table, label)) throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + label);
            // Label found!
            unit.symbol_table.push_back({label, Symbol::Type::TEXT, Symbol::Binding::LOCAL, instruction_address, true});
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

    // Process globals
    for (const auto &name : globals_to_process) {
        Symbol *sym = find_symbol_in_table(unit.symbol_table, name);
        if (sym) {
            sym->binding = Symbol::Binding::GLOBAL;
        } else {
            unit.symbol_table.push_back({name, Symbol::Type::TEXT, Symbol::Binding::GLOBAL, 0, false});
        }
    }

    // --- Pass 2: Parse instructions ---
    line_num = 0;
    section = CurrentSection::UNKNOWN;

    for (const auto& original_line : source_lines) {
        line_num++;
        std::string cleaned_line = trim(strip_comments(original_line));
        
        if (cleaned_line.empty() || cleaned_line.back() == ':') continue;

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

            if (mnemonic == "iconst") { int32_t val; if (!(ss >> val)) throw std::runtime_error("L" + std::to_string(line_num)); instr = std::make_unique<IConst>(val); }
            else if (mnemonic == "iadd") instr = std::make_unique<IAdd>();
            else if (mnemonic == "isub") instr = std::make_unique<ISub>();
            else if (mnemonic == "imul") instr = std::make_unique<IMul>();
            else if (mnemonic == "idiv") instr = std::make_unique<IDiv>();
            else if (mnemonic == "ret") instr = std::make_unique<Ret>();
            else if (mnemonic == "jmp") { std::string lbl; ss >> lbl; instr = std::make_unique<Jmp>(lbl); }
            else if (mnemonic == "invoke") { std::string lbl; int args; ss >> lbl >> args; instr = std::make_unique<Invoke>(lbl, (uint8_t)args); }
            else if (mnemonic == "istore") { int32_t idx; if (!(ss >> idx)) throw std::runtime_error("L" + std::to_string(line_num)); instr = std::make_unique<IStore>(idx); }
            else if (mnemonic == "iload") { int32_t idx; if (!(ss >> idx)) throw std::runtime_error("L" + std::to_string(line_num)); instr = std::make_unique<ILoad>(idx); }
            else if (mnemonic == "NEW_ARRAY") instr = std::make_unique<NewArray>();
            else if (mnemonic == "SET_ELEM") instr = std::make_unique<SetElem>();
            else if (mnemonic == "GET_ELEM") instr = std::make_unique<GetElem>();
            else if (mnemonic == "NEW_STRING") instr = std::make_unique<NewString>();
            else if (mnemonic == "SET_CHAR") instr = std::make_unique<SetChar>();
            else if (mnemonic == "GET_CHAR") instr = std::make_unique<GetChar>();
            else if (mnemonic == "icmp_eq") instr = std::make_unique<ICmpEQ>();
            else if (mnemonic == "icmp_lt") instr = std::make_unique<ICmpLT>();
            else if (mnemonic == "icmp_gt") instr = std::make_unique<ICmpGT>();
            else if (mnemonic == "jmp_if_false") { std::string lbl; ss >> lbl; instr = std::make_unique<JmpIfFalse>(lbl); }
            else if (mnemonic == "jnz") { std::string lbl; ss >> lbl; instr = std::make_unique<JmpIfNotZero>(lbl); }
            else if (mnemonic == "PRINT_I") instr = std::make_unique<PrintI>();
            else if (mnemonic == "PRINT_S") instr = std::make_unique<PrintS>();
            else throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic " + mnemonic);
            
            instr->source_line = trim(original_line);
            unit.instructions.push_back(std::move(instr));
        }
    }
    return unit;
}