# This Makefile should work on Linux, BSD, Mac, and the accursed Windows.
# This implies that it should be independent of make systems also. 
# Keep the system specific stuff out. Changing the compiler and such is a 
# user choice by setting the CC variable. For example, to use GNU components
# on OpenBSD:
#   doas pkg_add gmake egcc
#   gmake CC=egcc

# Other possible names: find-hosts, who-goes-there, lshosts, host-finder
PGM=hwaet
PGMD=$(PGM)d
WOL=wake-on-lan
OBJS=$(PGM)-common.o cross-platform-sockets.o string-ops.o
HDRS=$(PGM)-common.h cross-platform-sockets.h string-ops.h

# Turn on debugging symbols so that the executable can be debugged. This is
# required to be enabled in both the compiler and the linker.
CFLAGS=-g
LDFLAGS=-g

all: $(PGM) $(PGMD) $(WOL)


# $@ doesn't work on the dependency line with GNU make.
$(PGM): $(HDRS) $(OBJS) $(PGM).o
	$(CC) -o $@ $@.o $(OBJS)


# $@ doesn't work on the dependency line with GNU make.
$(PGMD): $(HDRS) $(OBJS) $(PGMD).o
	$(CC) -o $@ $@.o $(OBJS)

# $@ doesn't work on the dependency line with GNU make.
$(WOL): $(HDRS) cross-platform-sockets.o string-ops.o $(WOL).o
	$(CC) -o $@ $@.o cross-platform-sockets.o string-ops.o

clean:
	rm -f $(PGM) $(PGMD) $(WOL) *.o

winclean:
	for %%f in ($(PGM) $(PGMD) $(WOL) *.o) del %%f
        
        
install: $(PGM) $(PGMD) $(WOL)
	./install.sh

uninstall:
	./install.sh -u

