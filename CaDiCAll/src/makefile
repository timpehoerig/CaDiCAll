# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Target executable
TARGET = cadicall

# Source files
SRCS = cadicall.cpp ../utils/argparser.cpp ../utils/strings.cpp ../utils/parser.cpp ../../cadical/build/libcadical.a

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(filter-out ../../cadical/build/libcadical.a, $(OBJS)) $(TARGET) tmp*
