CXX      ?= g++
CXXFLAGS  = -std=c++23 -Wall -Wextra -Wpedantic -O2 -Isrc -pthread
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
    src/forth/forth.cpp \
    src/ada/ada.cpp

TEST_SRCS = \
    tests/test_main.cpp \
    src/brainfuck/brainfuck.cpp \
    src/math/bloom.cpp \
    src/achievements/achievements.cpp \
    src/algol/algol.cpp \
    src/fortran/fortran.cpp \
    src/prolog/prolog.cpp \
    src/forth/forth.cpp \
    src/ada/ada.cpp

TARGET         = build/hypertension
TEST_TARGET    = build/test_main
FORTRAN_TARGET = build/hypertension-confidence
ADA_TARGET     = build/hypertension-integrity
ADA_OBJ_DIR    = build/ada-objects

.PHONY: all test fortran ada clean

all: $(TARGET)

fortran: $(FORTRAN_TARGET)

ada: $(ADA_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TARGET): $(SRCS) | build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

$(TEST_TARGET): $(TEST_SRCS) | build
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $@

$(FORTRAN_TARGET): engine/confidence.f | build
	$(FC) $(FFLAGS) engine/confidence.f -o $@

$(ADA_TARGET): engine/integrity.adb | $(ADA_OBJ_DIR)
	(cd $(ADA_OBJ_DIR) && gnatmake -O2 ../../engine/integrity.adb -o ../hypertension-integrity)

$(ADA_OBJ_DIR):
	mkdir -p $(ADA_OBJ_DIR)

build:
	mkdir -p build

clean:
	rm -rf build
