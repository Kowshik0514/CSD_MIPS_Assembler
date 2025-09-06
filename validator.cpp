// File: validator.cpp
// Owner: Person A
// Role: Validator
// Description: C++ script to test the parser against all files in the tests/ directory.

#include "parser.h"
#include "structures.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> // Requires C++17

namespace fs = std::filesystem;

int main() {
    std::cout << "--- Running C++ Parser Validator ---" << std::endl;
    std::string tests_dir = "tests";
    int failed_count = 0;

    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        const auto& path = entry.path();
        if (path.extension() == ".stkasm") {
            std::cout << "--> Testing '" << path.string() << "'..." << std::endl;
            bool should_fail = path.filename().string().find("invalid") != std::string::npos;

            try {
                AssemblyUnit result = parse_file(path.string());
                if (should_fail) {
                    std::cout << "    \x1B[31mFAILED: Invalid file was parsed without errors.\x1B[0m" << std::endl;
                    failed_count++;
                } else {
                    std::cout << "    \x1B[32mPASSED: Parsed "
                              << result.instructions.size() << " instructions, "
                              << result.symbol_table.size() << " symbols, "
                              << result.data_entries.size() << " data entries.\x1B[0m"
                              << std::endl;
                }
            } catch (const std::runtime_error& e) {
                if (should_fail) {
                    std::cout << "    \x1B[32mPASSED: Invalid file correctly raised an error: "
                              << e.what() << "\x1B[0m" << std::endl;
                } else {
                    std::cout << "    \x1B[31mFAILED: Valid file raised an unexpected error: "
                              << e.what() << "\x1B[0m" << std::endl;
                    failed_count++;
                }
            }
        }
    }

    std::cout << "--- Validator Finished ---" << std::endl;
    return failed_count; // 0 = success, >0 = failures
}
