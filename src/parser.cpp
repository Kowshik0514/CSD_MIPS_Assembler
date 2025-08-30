// Description: Implementation of the parser for the .stkasm language.

#include "parser.h"
#include "symbol_table.h" // ** INCLUDE THE NEW SYMBOL TABLE **
#include <fstream>
#include <sstream>
#include <stdexcept>

// Helper function to trim whitespace from a string
std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first)
        return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// ** The return type is now the new SymbolTable class **
std::pair<std::vector<std::unique_ptr<Instruction>>, SymbolTable> parse_file(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filepath);
    }

    std::vector<std::unique_ptr<Instruction>> instructions;
    SymbolTable symbols; // ** Use the new SymbolTable object **
    std::string line;
    int line_num = 0;
    int instruction_count = 0; // Use a dedicated counter for instruction addresses

    // First Pass: Find all labels and count instructions
    while (std::getline(file, line))
    {
        line_num++;
        std::string cleaned_line = trim(line);
        if (cleaned_line.empty() || cleaned_line[0] == '#')
            continue;

        if (cleaned_line.back() == ':')
        {
            std::string label = cleaned_line.substr(0, cleaned_line.length() - 1);
            // ** Use the add_label method and check for duplicates **
            if (!symbols.add_label(label, instruction_count))
            {
                throw std::runtime_error("L" + std::to_string(line_num) + ": Duplicate symbol definition '" + label + "'.");
            }
        }
        else
        {
            // Only increment the instruction count for actual instruction lines
            instruction_count++;
        }
    }

    // Reset for second pass
    file.clear();
    file.seekg(0, std::ios::beg);
    line_num = 0;

    // Second Pass: Parse instructions into objects
    while (std::getline(file, line))
    {
        line_num++;

        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
        {
            line = line.substr(0, comment_pos);
        }
        line = trim(line);

        // Skip empty lines and label definitions
        if (line.empty() || line.back() == ':')
        {
            continue;
        }

        std::stringstream ss(line);
        std::string mnemonic;
        ss >> mnemonic;

        if (mnemonic == "iconst")
        {
            int32_t value;
            if (!(ss >> value))
                throw std::runtime_error("L" + std::to_string(line_num) + ": 'iconst' expects an integer argument.");
            instructions.push_back(std::make_unique<IConst>(value));
        }
        else if (mnemonic == "iadd")
        {
            instructions.push_back(std::make_unique<IAdd>());
        }
        // ** NEW PARSING LOGIC FOR MODULE 2 **
        else if (mnemonic == "isub")
        {
            instructions.push_back(std::make_unique<ISub>());
        }
        else if (mnemonic == "imul")
        {
            instructions.push_back(std::make_unique<IMul>());
        }
        else if (mnemonic == "idiv")
        {
            instructions.push_back(std::make_unique<IDiv>());
        }
        else if (mnemonic == "jmp")
        {
            std::string label;
            if (!(ss >> label))
                throw std::runtime_error("L" + std::to_string(line_num) + ": 'jmp' expects a label argument.");
            instructions.push_back(std::make_unique<Jmp>(label));
        }
        else if (mnemonic == "invoke")
        {
            std::string label;
            int num_args;
            if (!(ss >> label >> num_args))
                throw std::runtime_error("L" + std::to_string(line_num) + ": 'invoke' expects a label and a number of arguments.");
            instructions.push_back(std::make_unique<Invoke>(label, num_args));
        }
        else if (mnemonic == "ret")
        {
            instructions.push_back(std::make_unique<Ret>());
        }
        else
        {
            throw std::runtime_error("L" + std::to_string(line_num) + ": Unknown mnemonic '" + mnemonic + "'.");
        }
    }

    return {std::move(instructions), symbols};
}