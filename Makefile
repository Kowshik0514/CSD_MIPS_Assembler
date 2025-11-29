# File: Makefile
# Owner: Team
# Role: Build Script

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./src -g

# --- Target 1: The Assembler ---
ASSEMBLER_SRCS = assembler.cpp src/parser.cpp src/emitter.cpp
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
ASSEMBLER_TARGET = assembler

# --- Target 2: The Validator ---
VALIDATOR_SRCS = validator.cpp src/parser.cpp src/emitter.cpp
VALIDATOR_OBJS = $(VALIDATOR_SRCS:.cpp=.o)
VALIDATOR_TARGET = validator

# --- Target 3: The Linker ---
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
%.o: %.cpp $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f src/*.o *.o $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET) $(LINKER_TARGET) *.vm *.o

clean_outputs:
	rm -rf outputs
clean_linker_outputs:
	rm -rf linker_outputs