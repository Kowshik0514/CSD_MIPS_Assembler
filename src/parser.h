// Description: Header file for the parser function.

#ifndef PARSER_H
#define PARSER_H

#include "structures.h"
#include <string>
#include <vector>
#include <utility> // For std::pair
#include <memory>

// Parses a .stkasm file and returns a pair containing:
// 1. A vector of instruction objects.
// 2. The symbol table.
// Throws std::runtime_error on failure.
std::pair<std::vector<std::unique_ptr<Instruction>>, SymbolTable> parse_file(const std::string &filepath);

#endif // PARSER_H