#
# Makefile for the tiny mail system
#

CC = gcc
OBJS =	handler.o server.o client.o

tinyms: $(OBJS)
	$(CC) -o tinyms $(OBJS)

.c.o:
	$(CC) -Wall $(CFLAGS) -c $<

all:	tinyms

clean:
	rm -f tinyms *.o
