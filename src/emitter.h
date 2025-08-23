// File: emitter.h
// Owner: CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Header for the bytecode emitter function.

#ifndef EMITTER_H
#define EMITTER_H

#include "structures.h"
#include <vector>

// Takes instruction objects and a symbol table, and returns the final binary bytecode.
std::vector<uint8_t> emit_bytecode(
    const std::vector<std::unique_ptr<Instruction>>& instructions,
    const SymbolTable& symbols
);

#endif