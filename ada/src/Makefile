# Makefile for PGM

# gprbuild is not available on all platforms, like OpenBSD macppc, so we use gnatmake instead.

# The syntax of this Makefile is designed to work on both BSD and GNU make, particularly the
# '!=' assignment which collects the output of a shell command.

PGM=hwaet
PGM2=hwaetd
SHELL=/bin/sh
SRC=$(PGM).adb
OBJ=$(PGM).o
SRC2=$(PGM2).adb
OBJ2=$(PGM2).o
OS != uname -s
MACHINE != uname -m

# Relative to src directory where this Makefile exists.
BUILD_DIR=../obj
# Compile is performed in the build directory, so these are relative to it.
BIN_DIR=../bin
SRC_DIR=../src

NATIVE_OBJ=$(BUILD_DIR)/native_errors.o

GNAT_FLAGS = -g -E -gnatQ -gnat05 -gnatwa -gnato

# Use the GNU linker on OpenBSD macppc because the default CLANG linker causes
# a fatal error when linking the program: relocation refers to a discarded section
FORCE_GNU_LINKER != [ "$(OS)" = 'OpenBSD' ] && [ "$(MACHINE)" = 'macppc' ] && echo '-fuse-ld=bfd'
CC != [ "$(OS)" = 'OpenBSD' ] && [ "$(MACHINE)" = 'macppc' ] && echo 'egcc'

# This prevents the linker error:
#   /usr/bin/ld.bfd: Dwarf Error: found dwarf version '5',
#   this reader only handles version 2 information.
# The -gdwarf-2 absolutely has to be the last argument on the command line or
# one of the tools can't parse it properly and the compile fails.
CFLAGS=-gstrict-dwarf -gdwarf-2

LDFLAGS=$(FORCE_GNU_LINKER) $(NATIVE_OBJ)

all: $(PGM) $(PGM2)

$(PGM): $(SRC) $(BUILD_DIR) $(NATIVE_OBJ)
	cd $(BUILD_DIR) && gnatmake $(GNAT_FLAGS) -I$(SRC_DIR) $(SRC_DIR)/$(SRC) -o $(PGM) -largs $(LDFLAGS) -cargs $(CFLAGS) 2>&1 | $(BIN_DIR)/rm-misused-func-warnings.pl

$(PGM2): $(SRC2) $(BUILD_DIR) $(NATIVE_OBJ)
	cd $(BUILD_DIR) && gnatmake $(GNAT_FLAGS) -I$(SRC_DIR) $(SRC_DIR)/$(SRC2) -o $(PGM2) -largs $(LDFLAGS) -cargs $(CFLAGS) 2>&1 | $(BIN_DIR)/rm-misused-func-warnings.pl

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/native_errors.o:
	$(CC) -c -g -o $(BUILD_DIR)/native_errors.o native_errors.c
        
clean:
	rm -rf $(BUILD_DIR)

