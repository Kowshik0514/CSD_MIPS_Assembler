# File: Makefile
# Owner: Team
# Role: Build Script
# Description: Correctly compiles and links all toolchain components.

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./src -g # -g for debugging

# --- Target 1: The Assembler ---
# It needs the main file, the parser, and the emitter.
ASSEMBLER_SRCS = assembler.cpp src/parser.cpp src/emitter.cpp
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
ASSEMBLER_TARGET = assembler

# --- Target 2: The Validator ---
# It needs the validator main, the parser, and the emitter (for vtables).
VALIDATOR_SRCS = validator.cpp src/parser.cpp src/emitter.cpp
VALIDATOR_OBJS = $(VALIDATOR_SRCS:.cpp=.o)
VALIDATOR_TARGET = validator

# --- Target 3: The Linker ---
# It needs its main file and the linker logic file.
LINKER_SRCS = linker_main.cpp src/linker.cpp
LINKER_OBJS = $(LINKER_SRCS:.cpp=.o)
LINKER_TARGET = linker

# Default rule: build everything
all: $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET) $(LINKER_TARGET)

# Rule to link the assembler
$(ASSEMBLER_TARGET): $(ASSEMBLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link the validator
$(VALIDATOR_TARGET): $(VALIDATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link the linker
$(LINKER_TARGET): $(LINKER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule to compile any .cpp file into a .o file
# This depends on .h files, so it will recompile if a header changes.
%.o: %.cpp $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f src/*.o *.o $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET) $(LINKER_TARGET) *.vm *.o
# 	rm -rf outputs