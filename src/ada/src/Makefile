# Makefile for PGM

# gprbuild is not available on all platforms, like OpenBSD macppc, so we use gnatmake instead.

# The syntax of this Makefile is designed to work on both BSD and GNU make, particularly the
# '!=' assignment which collects the output of a shell command.

PGM=hwaet
SHELL=/bin/sh
SRC=$(PGM).adb
OBJ=$(PGM).o
OS != uname -s
MACHINE != uname -m

# Relative to src directory where this Makefile exists.
BUILD_DIR=../build
# Compile is performed in the build directory, so these are relative to it.
BIN_DIR=../bin
SRC_DIR=../src

# Use the GNU linker on OpenBSD macppc because the default CLANG linker causes
# a fatal error when linking the program: relocation refers to a discarded section
FORCE_GNU_LINKER != [ "$(OS)" = 'OpenBSD' ] && [ "$(MACHINE)" = 'macppc' ] && echo '-fuse-ld=bfd'
GNAT_FLAGS = -gnatQ -gnat05 -g -gnatwa -gnato $(FORCE_GNU_LINKER)

all: $(PGM)

$(PGM): $(SRC) $(BUILD_DIR)
	cd $(BUILD_DIR) && gnatmake $(GNAT_FLAGS) -I$(SRC_DIR) $(SRC_DIR)/$(SRC) -o $(PGM) 2>&1 | $(BIN_DIR)/rm-misused-func-warnings.pl

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
