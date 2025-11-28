// File: linker.h
// Owner: Your Team
// Role: Linker Logic Header
// Description: Defines the interface for the linker.

#ifndef LINKER_H
#define LINKER_H

#include "structures.h" // Includes ObjectFile and GlobalSymbolTable definitions
#include <vector>
#include <string>
#include <map>

// Struct to hold all final linked data, returned by the link_objects function
struct LinkedProgram {
    std::vector<uint8_t> vm_bytes; // The full binary for the .vm file
    GlobalSymbolTable symbol_table;
    uint32_t entry_point;
    uint32_t final_code_size;
    uint32_t final_data_size;
};

// The main function that performs the linking process.
// Takes a list of loaded object files.
// Returns a LinkedProgram struct with all the final data.
// Throws std::runtime_error on linking errors.
LinkedProgram link_objects(const std::vector<ObjectFile>& objects);

#endif // LINKER_H