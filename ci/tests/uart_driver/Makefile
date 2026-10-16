CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -pthread

# Mock headers first (Fw, Os, zephyr, generated component base), then the real driver sources
INCLUDES := -Imocks -I../../.. -I../../../default

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/uart_driver_rx_test
DRIVER    := ../../../fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.cpp

.PHONY: all clean test

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): test_uart_driver.cpp mocks/zephyr_mocks.cpp $(DRIVER) $(wildcard mocks/*.hpp mocks/*/*.h mocks/*/*.hpp mocks/*/*/*.hpp) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test_uart_driver.cpp mocks/zephyr_mocks.cpp $(DRIVER)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
