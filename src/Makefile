# GNU Make is required. Sometimes it is named gmake to differentiate from a platform's native make.
ifeq ($(OS),Windows_NT)
    EXE_EXT=.exe
else
    EXE_EXT=
    
    OS:=$(shell uname -s)
    CPU:=$(shell uname -p)

    ifeq ($(OS),OpenBSD)
        CC=egcc
    else
		ifeq ($(OS),Darwin)
			MAH_SHEEN:=$(shell machine)
			ifeq ($(CPU),powerpc)
				CFLAGS+=-fast
				ifeq ($(MAH_SHEEN),ppc7450)
					CFLAGS+=-mcpu=7450
				endif
			endif
	    endif
    endif
endif

CC=egcc
PGM=hwaet
PGMD=hwaetd
OBJS=hwaet-common.o
HDRS=hwaet-common.h

all: $(PGM) $(PGMD)


$(PGM): $(HDRS) $(OBJS) $@.o


$(PGMD): $(HDRS) $(OBJS) $@.c


clean:
	rm $(PGM) $(PGMD) *.o
	
