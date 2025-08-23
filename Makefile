# File: Makefile
# Owner: Team
# Role: Build Script
# Description: Compiles the C++ source files into two separate executables.

# Compiler and flags
CXX = g++
# Use C++17 standard, enable all warnings, include the 'src' directory
CXXFLAGS = -std=c++17 -Wall -I./src

# --- Target 1: The Assembler ---
ASSEMBLER_SRCS = assembler.cpp src/parser.cpp src/emitter.cpp
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
ASSEMBLER_TARGET = assembler

# --- Target 2: The Validator ---
VALIDATOR_SRCS = validator.cpp src/parser.cpp src/emitter.cpp
VALIDATOR_OBJS = $(VALIDATOR_SRCS:.cpp=.o)
VALIDATOR_TARGET = validator

# Default rule: build everything
all: $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET)

# Rule to link the assembler
$(ASSEMBLER_TARGET): $(ASSEMBLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link the validator
$(VALIDATOR_TARGET): $(VALIDATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule to compile any .cpp file into a .o file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f src/*.o *.o $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET)