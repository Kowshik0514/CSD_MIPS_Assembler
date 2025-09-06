# File: Makefile
# Owner: Team
# Role: Build Script
# Description: Compiles the C++ source files into two separate executables and
#              provides rules to assemble .stkasm files into .vm files.

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./src

# --- Target 1: The Assembler ---
ASSEMBLER_SRCS = assembler.cpp src/parser.cpp src/emitter.cpp src/symbol_table.cpp
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
ASSEMBLER_TARGET = assembler

# --- Target 2: The Validator ---
VALIDATOR_SRCS = validator.cpp src/parser.cpp src/emitter.cpp src/symbol_table.cpp
VALIDATOR_OBJS = $(VALIDATOR_SRCS:.cpp=.o)
VALIDATOR_TARGET = validator

# Find all .stkasm files in tests/
STKASM_FILES := $(wildcard tests/*.stkasm)
VM_FILES := $(STKASM_FILES:.stkasm=.vm)

# Default rule: build everything
all: $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET)

# Rule to build both executables
$(ASSEMBLER_TARGET): $(ASSEMBLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(VALIDATOR_TARGET): $(VALIDATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule to compile any .cpp file into a .o file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- New rule: Generate .vm from .stkasm using assembler ---
%.vm: %.stkasm $(ASSEMBLER_TARGET)
	./$(ASSEMBLER_TARGET) $< $@

# Rule: build all .vm files for all tests
vm: $(VM_FILES)

# Clean up build files

clean:
	rm -f src/*.o *.o $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET) $(VM_FILES) stdout output output.vm