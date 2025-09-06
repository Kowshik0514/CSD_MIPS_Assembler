// File: parser.cpp
// Owner: Nischith
// Role: Parser & Front-end
// Description: Implementation of the parser for the .stkasm language with sections and directives.

#include "parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::find_if

// Helper function to trim whitespace from a string
std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first)
        return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Helper to find a symbol in the symbol table being built.
Symbol *find_symbol_in_table(std::vector<Symbol> &table, const std::string &name)
{
    auto it = std::find_if(table.begin(), table.end(),
                           [&](const Symbol &sym)
                           { return sym.name == name; });
    return (it != table.end()) ? &(*it) : nullptr;
}

AssemblyUnit parse_file(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filepath);
    }

    AssemblyUnit unit;
    std::string line;
    int line_num = 0;

    enum class CurrentSection
    {
        TEXT,
        DATA,
        UNKNOWN
    };
    CurrentSection section = CurrentSection::UNKNOWN;

    uint32_t instruction_address = 0;
    uint32_t data_address = 0;

    std::vector<std::string> globals_to_process;

    // --- Pass 1: Identify all symbols (labels and data) and their locations. ---
    while (std::getline(file, line))
    {
        line_num++;
        std::string cleaned_line = trim(line);
        if (cleaned_line.empty() || cleaned_line[0] == '#')
            continue;

        // Handle directives
        if (cleaned_line[0] == '.')
        {
            std::stringstream ss(cleaned_line);
            std::string directive;
            ss >> directive;
            if (directive == ".text")
                section = CurrentSection::TEXT;
            else if (directive == ".data")
                section = CurrentSection::DATA;
            else if (directive == ".global")
            {
                std::string label_name;
                ss >> label_name;
                globals_to_process.push_back(label_name);
            }
            else if (directive == ".static")
            {
                if (section != CurrentSection::DATA)
                    throw std::runtime_error("L" + std::to_string(line_num) + ": .static can only be used in .data section");
                std::string var_name;
                ss >> var_name;
                if (find_symbol_in_table(unit.symbol_table, var_name))
                    throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + var_name);
                unit.symbol_table.push_back({var_name, Symbol::Type::DATA, Symbol::Binding::LOCAL, data_address});
                data_address += 4; // All static data is 4 bytes for now
            }
        }
        // Handle labels
        else if (cleaned_line.back() == ':')
        {
            if (section != CurrentSection::TEXT)
                throw std::runtime_error("L" + std::to_string(line_num) + ": Labels can only be defined in .text section");
            std::string label = cleaned_line.substr(0, cleaned_line.length() - 1);
            if (find_symbol_in_table(unit.symbol_table, label))
                throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol " + label);
            unit.symbol_table.push_back({label, Symbol::Type::TEXT, Symbol::Binding::LOCAL, instruction_address});
        }
        // Handle instructions (just count them in the first pass)
        else
        {
            if (section != CurrentSection::TEXT)
                throw std::runtime_error("L" + std::to_string(line_num) + ": Instructions can only be in .text section");
            instruction_address++;
        }
    }

    // After pass 1, process all the .global directives
    for (const auto &name : globals_to_process)
    {
        Symbol *sym = find_symbol_in_table(unit.symbol_table, name);
        if (!sym)
            throw std::runtime_error("Global symbol '" + name + "' was not defined.");
        sym->binding = Symbol::Binding::GLOBAL;
    }

    // --- Pass 2: Parse instructions and data values ---
    file.clear();
    file.seekg(0, std::ios::beg);
    line_num = 0;
    section = CurrentSection::UNKNOWN;

    while (std::getline(file, line))
    {
        line_num++;
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);
        std::string cleaned_line = trim(line);
        if (cleaned_line.empty() || cleaned_line.back() == ':')
            continue;

        if (cleaned_line[0] == '.')
        {
            std::stringstream ss(cleaned_line);
            std::string directive;
            ss >> directive;
            if (directive == ".text")
                section = CurrentSection::TEXT;
            else if (directive == ".data")
                section = CurrentSection::DATA;
            else if (directive == ".static")
            {
                std::string var_name;
                int32_t value;
                ss >> var_name >> value;
                unit.data_entries.push_back({var_name, value});
            }
        }
        else if (section == CurrentSection::TEXT)
        {
            std::stringstream ss(cleaned_line);
            std::string mnemonic;
            ss >> mnemonic;

            if (mnemonic == "iconst")
            {
                int32_t value;
                ss >> value;
                unit.instructions.push_back(std::make_unique<IConst>(value));
            }
            else if (mnemonic == "iadd")
            {
                unit.instructions.push_back(std::make_unique<IAdd>());
            }
            else if (mnemonic == "isub")
            {
                unit.instructions.push_back(std::make_unique<ISub>());
            }
            else if (mnemonic == "imul")
            {
                unit.instructions.push_back(std::make_unique<IMul>());
            }
            else if (mnemonic == "idiv")
            {
                unit.instructions.push_back(std::make_unique<IDiv>());
            }
            else if (mnemonic == "jmp")
            {
                std::string label;
                ss >> label;
                unit.instructions.push_back(std::make_unique<Jmp>(label));
            }
            else if (mnemonic == "invoke")
            {
                std::string label;
                int num_args;
                ss >> label >> num_args;
                unit.instructions.push_back(std::make_unique<Invoke>(label, num_args));
            }
            else if (mnemonic == "ret")
            {
                unit.instructions.push_back(std::make_unique<Ret>());
            }
            else
            {
                throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
            }
        }
    }

    return unit;
}