# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./src

# --- Target 1: The Assembler ---
ASSEMBLER_SRCS = assembler.cpp src/parser.cpp src/emitter.cpp
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
ASSEMBLER_TARGET = assembler

# --- Target 2: The Validator ---
VALIDATOR_SRCS = validator.cpp src/parser.cpp src/emitter.cpp
VALIDATOR_OBJS = $(VALIDATOR_SRCS:.cpp=.o)
VALIDATOR_TARGET = validator

# --- Source files for assembly in tests folder ---
ASM_FOLDER = tests
ASM_FILES = $(wildcard $(ASM_FOLDER)/*.stkasm)
VM_FILES = $(ASM_FILES:$(ASM_FOLDER)/%.stkasm=%.vm)

# Default: build executables
all: $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET)

# Build assembler
$(ASSEMBLER_TARGET): $(ASSEMBLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build validator
$(VALIDATOR_TARGET): $(VALIDATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile any .cpp file to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generate .vm files from .stkasm files in tests folder
%.vm: $(ASM_FOLDER)/%.stkasm $(ASSEMBLER_TARGET)
	./$(ASSEMBLER_TARGET) $< $@

# Optional: build all VM files
.PHONY: vm
vm: $(VM_FILES)

# Clean build artifacts
clean:
	rm -f src/*.o *.o $(ASSEMBLER_TARGET) $(VALIDATOR_TARGET) *.vm stdout
