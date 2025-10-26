// File: linker_main.cpp
// Owner: Your Team (e.g., Nishchith)
// Role: Linker Driver Program
// Description: Parses arguments, reads .o files, invokes the linker, writes .vm file.

#include "structures.h" // Includes ObjectFile definition
#include "linker.h"     // Includes link_objects definition
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    // Basic argument parsing: linker input1.o input2.o ... -o output.vm
    if (argc < 4 || std::string(argv[argc - 2]) != "-o") {
        std::cerr << "Usage: " << argv[0] << " <input1.o> [input2.o ...] -o <output.vm>" << std::endl;
        return 1;
    }

    std::vector<std::string> input_files;
    for (int i = 1; i < argc - 2; ++i) {
        input_files.push_back(argv[i]);
    }
    std::string output_file = argv[argc - 1];

    std::cout << "Linking " << input_files.size() << " object file(s) -> '" << output_file << "'..." << std::endl;

    try {
        // 1. Read all specified object files into memory
        std::vector<ObjectFile> objects;
        objects.reserve(input_files.size()); // Optimize vector allocation
        for (const auto& file_path : input_files) {
            std::cout << "   Reading object file: " << file_path << std::endl;
            objects.push_back(ObjectFile::read_from(file_path));
        }

        // 2. Call the main linker function
        std::cout << "   Performing linking and relocation..." << std::endl;
        std::vector<uint8_t> executable_bytecode = link_objects(objects);
        std::cout << "   Linking successful. Final executable size: " << executable_bytecode.size() << " bytes." << std::endl;

        // 3. Write the final executable file
        std::ofstream outfile(output_file, std::ios::binary);
        if (!outfile) {
            throw std::runtime_error("Cannot open output file '" + output_file + "' for writing.");
        }
        outfile.write(reinterpret_cast<const char*>(executable_bytecode.data()), executable_bytecode.size());
        outfile.close(); // Ensure file is closed properly

        std::cout << "✅ Linker finished successfully." << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "❌ Linker failed: " << e.what() << std::endl;
        return 1; // Indicate failure
    }

    return 0; // Indicate success
}