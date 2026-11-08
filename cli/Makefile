# Makefile for nabla — GNU/Linux out-of-source build.
#
#   make                                                  release build into build/
#   make LENGTH_SIZE=64 BUILD_DIR=build64                 64-bit build into build64/
#   make BUILD_DIR=build-debug CXXFLAGS="-std=c++20 -O0 -g -Wall -Wextra"
#                                                         debug build into build-debug/
#   make clean                                            remove the default build/
#   make BUILD_DIR=build-debug clean                      remove a named build directory
#
# The executable lands at $(BUILD_DIR)/nabla — or $(BUILD_DIR)/nabla64 when
# LENGTH_SIZE=64 — and the object files at $(BUILD_DIR)/*.o, so no artifacts are
# written beside the sources.  Each
# compile also emits a build/*.d header-dependency file (DEPFLAGS), which
# the -include below pulls in so a header edit rebuilds its dependents.
#
# The tool sources in this directory are linked with the shared library
# sources in ../lib/ (the LCS library extracted from nabla).
#
# CXX, CPPFLAGS, and CXXFLAGS honor environment overrides and are also
# overridable on the make command line (GNU convention); DEPFLAGS is
# fixed (:=) and not meant to be overridden.

CXX      ?= g++
CPPFLAGS ?= -I../lib
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
DEPFLAGS := -MMD -MP

# Position index width: `make LENGTH_SIZE=64` builds a specialized release
# with 64-bit positions (LENGTH_SIZE_64); the default is 32-bit.
LENGTH_SIZE ?= 32
ifeq ($(LENGTH_SIZE),64)
CPPFLAGS += -DLENGTH_SIZE_64
endif

BUILD_DIR ?= build
SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
LIB_SOURCES := $(wildcard ../lib/*.cpp ../lib/*/*.cpp)
LIB_OBJECTS := $(patsubst ../lib/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SOURCES))
DEPS    := $(OBJECTS:.o=.d) $(LIB_OBJECTS:.o=.d)
TARGET  := $(BUILD_DIR)/$(if $(filter 64,$(LENGTH_SIZE)),nabla64,nabla)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS) $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../lib/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pull in the generated .d files so header edits rebuild their dependents.
-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
