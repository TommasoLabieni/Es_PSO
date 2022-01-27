IDIR = ./include/
CC = gcc
CFLAGS = -Wall -o
DEPS = $(IDIR)/list.h
OBJ = *.o
TARGET = search_insert_delete

all: $(TARGET) clean

list.o:		$(IDIR)list.c
				$(CC) -c $< 

$(TARGET):	$(TARGET).c list.o
				$(CC) $(CFLAGS) $@ $^ -lpthread

clean:
	rm $(OBJ)
