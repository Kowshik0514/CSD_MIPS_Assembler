// File: parser.h
// Owner: Nischith
// Role: Parser & Front-end
// Description: Header file for the parser function.

#ifndef PARSER_H
#define PARSER_H

#include "structures.h"
#include <string>

// Parses a .stkasm file and returns a complete AssemblyUnit object,
// which contains instructions, data, and symbol table information.
// Throws std::runtime_error on failure.
AssemblyUnit parse_file(const std::string &filepath);

#endif // PARSER_H