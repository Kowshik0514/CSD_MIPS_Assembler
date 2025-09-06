// File: CSD_MIPS_Linker/src/linker.cpp
// Role: The main program for the linker.

#include "linker_structures.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <output_file.vm> <input1.o> <input2.o> ..." << std::endl;
        return 1;
    }

    std::string output_file = argv[1];
    std::vector<std::string> input_files;
    for (int i = 2; i < argc; ++i) {
        input_files.push_back(argv[i]);
    }

    std::cout << "Linking " << input_files.size() << " object file(s) -> '" << output_file << "'..." << std::endl;

    try {
        std::vector<ParsedObjectFile> object_files;
        for (const auto& file_path : input_files) {
            std::cout << "Parsing '" << file_path << "'..." << std::endl;
            // The from_file method does all the parsing work for one file.
            object_files.push_back(ParsedObjectFile::from_file(file_path));
        }

        std::cout << "Parsing complete." << std::endl;
        std::cout << "Module 3 goal achieved: All object files have been successfully parsed." << std::endl;
        std::cout << "Next steps (Module 4 & 5): Symbol resolution, relocation, and final emission." << std::endl;


    } catch (const std::runtime_error& e) {
        std::cerr << "Linking failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}