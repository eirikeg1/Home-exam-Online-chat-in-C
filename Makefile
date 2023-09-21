CFLAGS = -g -std=gnu11 -Wall -Wextra
VFLAGS = --track-origins=yes --malloc-fill=0x40 --leak-check=full --show-leak-kinds=all --free-fill=0x23 -s
PORT = 2050
IP  = 172.29.71.124
CLIENTARGS = nick1 $(IP) $(PORT) 1 20
CLIENTARGS2 = nick2 $(IP) $(PORT) 1 20
BIN = upush_server upush_client


all: client server

client: upush_client.c
	gcc $(CFLAGS) upush_client.c -o upush_client -lm

run_client: client
	./upush_client $(CLIENTARGS)

run_client2: client
	./upush_client $(CLIENTARGS2)

vclient: client
	valgrind $(VFLAGS) ./upush_client $(CLIENTARGS)

server: upush_server.c send_packet.c
	gcc $(CFLAGS) upush_server.c -o upush_server -lm

run_server: server
	./upush_server $(PORT) 0

vserver: server
	valgrind $(VFLAGS) ./upush_server $(PORT) 0

clean:
	rm -f $(BIN)