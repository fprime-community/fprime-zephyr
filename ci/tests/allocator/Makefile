CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror

# Mock headers first (Fw/Types, zephyr/kernel.h), then the real allocator header
INCLUDES := -Imocks -I../../../default/zephyr-config

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/memory_allocator_smoke

.PHONY: all clean test

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): test_allocator.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
