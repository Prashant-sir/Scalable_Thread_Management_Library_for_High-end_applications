# ============================================================
# Makefile - Scalable Thread Management Library
# Target: Linux (GCC + pthreads)
# IDE: CodeBlocks (via Makefile target)
# ============================================================

# Compiler and flags
CC          = gcc
CFLAGS      = -Wall -Wextra -Wpedantic -std=c11 -O2
CFLAGS_DEBUG = -Wall -Wextra -Wpedantic -std=c11 -g -O0 -DDEBUG
LDFLAGS     = -lpthread -lrt

# Directories
INC_DIR     = include
SRC_DIR     = src
TEST_DIR    = tests
EXAMPLE_DIR = examples
BUILD_DIR   = build

# Source files
LIB_SOURCES = $(SRC_DIR)/core/task.c \
              $(SRC_DIR)/core/safe_queue.c \
              $(SRC_DIR)/core/thread_pool.c \
              $(SRC_DIR)/utils/logger.c

TEST_SOURCES = $(TEST_DIR)/test_basic.c \
               $(TEST_DIR)/test_stress.c \
               $(TEST_DIR)/test_shutdown.c

EXAMPLE_SOURCES = $(EXAMPLE_DIR)/demo.c

# Object files
LIB_OBJECTS  = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SOURCES))
TEST_BIN     = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))
EXAMPLE_BIN  = $(patsubst $(EXAMPLE_DIR)/%.c,$(BUILD_DIR)/%,$(EXAMPLE_SOURCES))

# Include path
INCLUDES = -I$(INC_DIR)

# ============================================================
# Default target
# ============================================================
.PHONY: all clean dirs test test_basic test_stress test_shutdown demo debug help

all: dirs $(TEST_BIN) $(EXAMPLE_BIN)
	@echo ""
	@echo "Build complete! Binaries in $(BUILD_DIR)/"
	@echo "  Tests:    test_basic, test_stress, test_shutdown"
	@echo "  Example:  demo"
	@echo ""
	@echo "Run 'make test' to execute all tests"
	@echo "Run 'make demo' to run the demo"

# ============================================================
# Directory creation
# ============================================================
dirs:
	@mkdir -p $(BUILD_DIR)/core $(BUILD_DIR)/utils

# ============================================================
# Pattern rules
# ============================================================

# Library object files
$(BUILD_DIR)/core/%.o: $(SRC_DIR)/core/%.c | dirs
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c | dirs
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test executables
$(BUILD_DIR)/test_basic: $(TEST_DIR)/test_basic.c $(LIB_OBJECTS) | dirs
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_stress: $(TEST_DIR)/test_stress.c $(LIB_OBJECTS) | dirs
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_shutdown: $(TEST_DIR)/test_shutdown.c $(LIB_OBJECTS) | dirs
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) -o $@ $(LDFLAGS)

# Example executable
$(BUILD_DIR)/demo: $(EXAMPLE_DIR)/demo.c $(LIB_OBJECTS) | dirs
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) -o $@ $(LDFLAGS)

# ============================================================
# Debug build
# ============================================================
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: dirs $(TEST_BIN) $(EXAMPLE_BIN)
	@echo ""
	@echo "Debug build complete (with -g -O0)"

# ============================================================
# Test targets
# ============================================================
test: all
	@echo ""
	@echo "========================================"
	@echo "  Running All Tests"
	@echo "========================================"
	@$(BUILD_DIR)/test_basic
	@$(BUILD_DIR)/test_stress
	@$(BUILD_DIR)/test_shutdown
	test_basic: $(BUILD_DIR)/test_basic
	@$(BUILD_DIR)/test_basic

test_stress: $(BUILD_DIR)/test_stress
	@$(BUILD_DIR)/test_stress

test_shutdown: $(BUILD_DIR)/test_shutdown
	@$(BUILD_DIR)/test_shutdown

# ============================================================
# Demo target
# ============================================================
demo: $(BUILD_DIR)/demo
	@echo ""
	@echo "========================================"
	@echo "  Running Demo"
	@echo "========================================"
	@$(BUILD_DIR)/demo

# ============================================================
# Clean
# ============================================================
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"

# ============================================================
# Help
# ============================================================
help:
	@echo ""
	@echo "Scalable Thread Management Library - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all tests and examples (default)"
	@echo "  debug        - Build with debug symbols and no optimization"
	@echo "  test         - Run all tests"
	@echo "  test_basic   - Run basic functionality tests"
	@echo "  test_stress  - Run stress/performance tests"
	@echo "  test_shutdown- Run shutdown behavior tests"
	@echo "  demo         - Run demonstration program"
	@echo "  clean        - Remove all build artifacts"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC=$(CC)"
	@echo "  CFLAGS=$(CFLAGS)"
	@echo ""
