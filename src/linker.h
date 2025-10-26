// File: linker.h
// Owner: Your Team (e.g., Rashmitha)
// Role: Linker Logic Header
// Description: Defines the interface for the linker.

#ifndef LINKER_H
#define LINKER_H

#include "structures.h" // Includes ObjectFile and GlobalSymbolTable definitions
#include <vector>
#include <string>

// The main function that performs the linking process.
// Takes a list of loaded object files.
// Returns a byte vector representing the final .vm executable content.
// Throws std::runtime_error on linking errors (duplicate symbols, undefined symbols).
std::vector<uint8_t> link_objects(const std::vector<ObjectFile>& objects);

#endif // LINKER_H