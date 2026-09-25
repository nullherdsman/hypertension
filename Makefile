CXX      ?= g++
CXXFLAGS  = -std=c++23 -Wall -Wextra -Wpedantic -O2 -Isrc
FC        = gfortran
FFLAGS    = -O2

SRCS = \
    src/main.cpp \
    src/cli/cli.cpp \
    src/runtime/mode.cpp \
    src/brainfuck/brainfuck.cpp \
    src/math/bloom.cpp \
    src/achievements/achievements.cpp \
    src/algol/algol.cpp \
    src/fortran/fortran.cpp \
    src/prolog/prolog.cpp \
    src/forth/forth.cpp

TEST_SRCS = \
    tests/test_main.cpp \
    src/brainfuck/brainfuck.cpp \
    src/math/bloom.cpp \
    src/achievements/achievements.cpp \
    src/algol/algol.cpp \
    src/fortran/fortran.cpp \
    src/prolog/prolog.cpp \
    src/forth/forth.cpp

TARGET         = build/hypertension
TEST_TARGET    = build/test_main
FORTRAN_TARGET = build/hypertension-confidence

.PHONY: all test fortran clean

all: $(TARGET)

fortran: $(FORTRAN_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TARGET): $(SRCS) | build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

$(TEST_TARGET): $(TEST_SRCS) | build
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $@

$(FORTRAN_TARGET): engine/confidence.f | build
	$(FC) $(FFLAGS) engine/confidence.f -o $@

build:
	mkdir -p build

clean:
	rm -rf build
