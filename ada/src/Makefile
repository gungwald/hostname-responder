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
BUILD_DIR=../obj
# Compile is performed in the build directory, so these are relative to it.
BIN_DIR=../bin
SRC_DIR=../src

GNAT_FLAGS = -g -Es -gnatQ -gnat05 -gnatwa -gnato

# Use the GNU linker on OpenBSD macppc because the default CLANG linker causes
# a fatal error when linking the program: relocation refers to a discarded section
FORCE_GNU_LINKER != [ "$(OS)" = 'OpenBSD' ] && [ "$(MACHINE)" = 'macppc' ] && echo '-fuse-ld=bfd'

# This prevents the linker error:
#   /usr/bin/ld.bfd: Dwarf Error: found dwarf version '5',
#   this reader only handles version 2 information.
# The -gdwarf-2 absolutely has to be the last argument on the command line or
# one of the tools can't parse it properly and the compile fails.
CFLAGS=-gstrict-dwarf -gdwarf-2

LDFLAGS=$(FORCE_GNU_LINKER)

all: $(PGM)

$(PGM): $(SRC) $(BUILD_DIR)
	cd $(BUILD_DIR) && gnatmake $(GNAT_FLAGS) -I$(SRC_DIR) $(SRC_DIR)/$(SRC) -o $(PGM) -largs $(LDFLAGS) -cargs $(CFLAGS) 2>&1 | $(BIN_DIR)/rm-misused-func-warnings.pl

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

