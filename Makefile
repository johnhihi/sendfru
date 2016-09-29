RM = rm -f

default: all

all: sendfru

sendfru: sendfru.c sendfru.h
	$(CC) $(CFLAGS) -o sendfru sendfru.c

clean:
	$(RM) sendfru

