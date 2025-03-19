CC = gcc
CFLAGS = -Iinclude
LDFLAGS = -Llib -lws2811

all: control_test

control_test: src/main.c
	$(CC) $(CFLAGS) -o control_test src/main.c $(LDFLAGS) -lm -lpthread

clean:
	rm -f control_test