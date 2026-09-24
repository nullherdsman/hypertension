CXX      ?= g++
CXXFLAGS  = -std=c++23 -Wall -Wextra -Wpedantic -O2 -Isrc

SRCS = \
    src/main.cpp \
    src/cli/cli.cpp \
    src/runtime/mode.cpp \
    src/brainfuck/brainfuck.cpp \
    src/math/bloom.cpp \
    src/achievements/achievements.cpp

TEST_SRCS = \
    tests/test_main.cpp \
    src/brainfuck/brainfuck.cpp \
    src/math/bloom.cpp \
    src/achievements/achievements.cpp

TARGET      = build/hypertension
TEST_TARGET = build/test_main

.PHONY: all test clean

all: $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TARGET): $(SRCS) | build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

$(TEST_TARGET): $(TEST_SRCS) | build
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $@

build:
	mkdir -p build

clean:
	rm -rf build
