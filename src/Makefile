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
	
