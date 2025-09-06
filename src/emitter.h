// File: emitter.h
// Owner: CS22B015 Kowshik
// Role: Bytecode & Back-end
// Description: Header for the object file emitter function.

#ifndef EMITTER_H
#define EMITTER_H

#include "structures.h"
#include <vector>
#include <cstdint>

// Takes a complete AssemblyUnit and returns the binary for a relocatable object file (.o).
std::vector<uint8_t> emit_object_file(const AssemblyUnit &unit);

#endif