ifeq ($(CROSS_COMPILE),)
    CC = gcc
    EXEC = writer
else
    CC = $(CROSS_COMPILE)gcc
    EXEC = writer_cross
endif

CFLAGS = -Wall -Werror
TARGET = $(EXEC)

all: $(TARGET)

writer: writer.o
	$(CC) $(CFLAGS) -o writer writer.o

writer_cross: writer.o
	$(CC) $(CFLAGS) -o writer_cross writer.o

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c

clean:
	rm -f *.o writer writer_cross
