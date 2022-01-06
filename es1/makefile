IDIR = ./include/
CC = gcc
CFLAGS = -Wall -o
DEPS = $(IDIR)/arrayOperation.h
OBJ = *.o
TARGET = hilzer_barbershop

all: $(TARGET) clean

arrayOperation.o:	$(IDIR)arrayOperation.c
					$(CC) -c $< 

$(TARGET):			$(TARGET).c arrayOperation.o
					$(CC) $(CFLAGS) $@ $^ -lpthread

clean:
		rm $(OBJ)

