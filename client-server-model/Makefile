# Makefile to compile the client-server model
CC=gcc

compile-server:
	$(CC) -o tcp_server_chat tcp_server_chat.c

compile-client:
	$(CC) -o tcp_client_chat tcp_client.c

compile-server-debug:
	$(CC) -o tcp_server_chat -D DEBUG tcp_server_chat.c

compile-client-debug:
	$(CC) -o tcp_client_chat -D DEBUG tcp_client.c

run-server:
	./tcp_server_chat 47783

run-client:
	./tcp_client_chat localhost 47783

clean:
	rm -f tcp_server_chat tcp_client_chat
