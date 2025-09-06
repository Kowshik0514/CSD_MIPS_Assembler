# File: CSD_MIPS_Linker/Makefile
# Makefile for the Linker

CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

SRCS = src/linker.cpp src/object_parser.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = linker

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)