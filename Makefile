# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinc
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = inc
BUILD_DIR = build

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/tf_interpreter

# Default target
build: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run with test file
run: $(TARGET)
	./$(TARGET) tests/giovanni.tf

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Release build with optimizations
release: CFLAGS += -O3 -flto
release: clean $(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  make [build]  - Build project"
	@echo "  make run      - Build and run with test file"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make release  - Build optimized release"
	@echo "  make help     - Show this help"

.PHONY: build run clean debug release help
