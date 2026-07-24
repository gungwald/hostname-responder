# ==============================================================================
# Makefile for Vladimir Makarov m2c Modula-2 Compiler (Dual-Stack Sockets Layer)
# ==============================================================================

# Compilers and Drivers
M2C       := m2c
CC        := gcc

# Flags (Enable optimizations and safety warnings for the native layer)
M2CFLAGS  := -v
CFLAGS    := -O2 -Wall -c
LDFLAGS   := 

# Application Target Binary
TARGET    := network_service

# Complete Source Blueprint
DEFS      := Socket.def Sockets.def
MODS      := Socket.mod Sockets.mod app.mod

# Auto-Generated C Intermediates & Binary Object Targets
GEN_C_SRCS := Socket.c Sockets.c app.c
GEN_C_OBJS := Socket.o Sockets.o app.o

# Phony targets to protect against matching filenames
.PHONY: all clean purge

# 1. Default Target: Links all machine code object packages into final executable
all: $(TARGET)

$(TARGET): $(GEN_C_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# 2. Native Compilation Phase: Converts auto-generated C text files into binary blobs
Socket.o: Socket.c
	$(CC) $(CFLAGS) -o $@ $<

Sockets.o: Sockets.c
	$(CC) $(CFLAGS) -o $@ $<

app.o: app.c
	$(CC) $(CFLAGS) -o $@ $<

# 3. Modula-2 Translation Phase: Translates Modula-2 sources into standard POSIX C
# (Makarov's m2c reads the corresponding .def headers automatically during compilation)
Socket.c: Socket.mod Socket.def
	$(M2C) $(M2CFLAGS) -m Socket.mod

Sockets.c: Sockets.mod Sockets.def Socket.def
	$(M2C) $(M2CFLAGS) -m Sockets.mod

app.c: app.mod Sockets.def Socket.def
	$(M2C) $(M2CFLAGS) -m app.mod

# 4. Clean Workspace: Removes local executable and object code files
clean:
	rm -f $(TARGET) $(GEN_C_OBJS)

# 5. Full Workspace Purge: Sweeps everything away, including auto-generated C code templates
purge: clean
	rm -f $(GEN_C_SRCS)
