CC=egcc
PGM=hwaet
PGMD=hwaetd
OBJS=hwaet-common.o
HDRS=hwaet-common.h

all: $(PGM) $(PGMD)


$(PGM): $(HDRS) $(OBJS) $@.o
	$(CC) -o $(PGM) $@.o $(OBJS)


$(PGMD): $(HDRS) $(OBJS) $@.o
	$(CC) -o $(PGMD) $@.o $(OBJS)


clean:
	rm $(PGM) $(PGMD) *.o
	
