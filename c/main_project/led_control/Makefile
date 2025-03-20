CC = gcc
CFLAGS = -Iinclude
WEBFLAGS = -I../../libwebsockets/include
LDFLAGS = -Llib -lws2811 

all: control_test

server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c -lm -lpthread

control_test: src/main.c
	$(CC) $(CFLAGS) -o control_test src/main.c $(LDFLAGS) -lm -lpthread

clean:
	rm -f control_test
	rm -f server