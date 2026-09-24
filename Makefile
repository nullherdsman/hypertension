CXX      ?= g++
CXXFLAGS  = -std=c++23 -Wall -Wextra -Wpedantic -O2 -Isrc
SRCS      = src/main.cpp src/cli/cli.cpp src/runtime/mode.cpp
TARGET    = build/hypertension

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) | build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

build:
	mkdir -p build

clean:
	rm -rf build
