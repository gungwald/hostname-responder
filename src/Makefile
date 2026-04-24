# This Makefile should work on Linux, BSD, Mac, and the accursed Windows. 
# Keep the system specific stuff out. Changing the compiler and such is a 
# user choice by setting the CC variable. For example, to use GNU components
# on OpenBSD:
#   doas pkg_add gmake egcc
#   gmake CC=egcc

# Other possible names: find-hosts, who-goes-there, lshosts, host-finder
PGM=hwaet
PGMD=$(PGM)d
OBJS=$(PGM)-common.o string-ops.o
HDRS=$(PGM)-common.h cross-platform-sockets.h string-ops.h

all: $(PGM) $(PGMD)


# $@ doesn't work on the dependency line with GNU make.
$(PGM): $(HDRS) $(OBJS) $(PGM).o
	$(CC) -o $@ $@.o $(OBJS)


# $@ doesn't work on the dependency line with GNU make.
$(PGMD): $(HDRS) $(OBJS) $(PGMD).o
	$(CC) -o $@ $@.o $(OBJS)


clean:
	rm -f $(PGM) $(PGMD) *.o

wclean:
	for %%f in ($(PGM) $(PGMD) *.o) del %%f
