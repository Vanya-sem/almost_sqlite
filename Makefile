CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = database
SOURCES = db_main.cpp database.cpp storage_manager.cpp query_executor.cpp sql_parser.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

