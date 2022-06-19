# Makefile to compile the client-server model
CC=gcc

server:
	$(CC) -o tcp_server_chat tcp_server_chat.c

client:
	$(CC) -o tcp_client_chat tcp_client.c

server-debug:
	$(CC) -o tcp_server_chat -D DEBUG -g3 tcp_server_chat.c

client-debug:
	$(CC) -o tcp_client_chat -D DEBUG -g3 tcp_client.c

run-server:
	./tcp_server_chat 47784

run-client:
	./tcp_client_chat localhost 47784

clean:
	rm -f tcp_server_chat tcp_client_chat
