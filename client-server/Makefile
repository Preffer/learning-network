CC		= g++ -std=c++11
CFLAGS		= -Wall -O3
LINKFLAGS	=

all: client server

client: client.cpp
	$(CC) $(CFLAGS) $(LINKFLAGS) client.cpp -o client

server: server.cpp
	$(CC) $(CFLAGS) $(LINKFLAGS) server.cpp -o server

clean:
	rm client server
