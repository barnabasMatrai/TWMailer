# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g -O0 -Iinclude

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# Targets
CLIENT := $(BIN_DIR)/twmailer-client
SERVER := $(BIN_DIR)/twmailer-server

# Source files
CLIENT_SRC := $(SRC_DIR)/TWMailerClient.cpp twmailer-client.cpp
SERVER_SRC := $(SRC_DIR)/TWMailerServer.cpp twmailer-server.cpp

# Object files (placed in obj/)
CLIENT_OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SRC))
SERVER_OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SERVER_SRC))

# Default target
all: $(CLIENT) $(SERVER)

# Build client executable
$(CLIENT): $(CLIENT_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build server executable
$(SERVER): $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule for compiling .cpp -> .o
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object and binary files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Convenience shortcuts
run-client: $(CLIENT)
	./$(CLIENT)

run-server: $(SERVER)
	./$(SERVER)

.PHONY: all clean run-client run-server
