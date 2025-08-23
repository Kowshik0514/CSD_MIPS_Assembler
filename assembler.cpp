// Description: The main program that assembles a .stkasm file into a .vm file.

#include "parser.h"
#include "emitter.h"
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[])
{
    // Check for correct command-line arguments
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file.stkasm> <output_file.vm>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    std::cout << "Assembling '" << input_file << "' -> '" << output_file << "'..." << std::endl;

    try
    {
        // Step 1: Parse the input file 
        auto [instructions, symbol_table] = parse_file(input_file);
        std::cout << "Parsing successful: Found " << instructions.size() << " instructions." << std::endl;

        // Step 2: Emit the bytecode 
        std::vector<uint8_t> bytecode = emit_bytecode(instructions, symbol_table);
        std::cout << "Bytecode emission successful: Generated " << bytecode.size() << " bytes." << std::endl;

        // Step 3: Write the bytecode to the output file
        std::ofstream outfile(output_file, std::ios::binary);
        if (!outfile)
        {
            throw std::runtime_error("Cannot open output file for writing.");
        }
        outfile.write(reinterpret_cast<const char *>(bytecode.data()), bytecode.size());

        std::cout << "Assembly complete." << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Assembly failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}