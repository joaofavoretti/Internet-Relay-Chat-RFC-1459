#include "../shared/socket_connection.h"
#include "../shared/logger.h"


#include <ctype.h>
#include <stdio.h>
#include <string.h>


int max(int a, int b)
{
  return a > b ? a : b;
}

void transmit_message(SOCKET message_sender, char *message, size_t message_length, fd_set *channelset, int nfds) {
  char sender_boilerplate[128];
  int boilerplate_size = sprintf(sender_boilerplate, "Client %d: ", message_sender);
  
  for (SOCKET i = 1; i < nfds; i++) {
    if (FD_ISSET(i, channelset) && i != message_sender) {
      send(i, sender_boilerplate, boilerplate_size, 0);
      send(i, message, message_length, 0);
    }
  }
}


int main(int argc, char *argv[])
{
  char *program_name = argv[0];

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s local_port\n", program_name);
    return 1;
  }

  char *local_port = argv[1];


  logger("Configuring local address...\n");
  struct addrinfo hints = { 0 };
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo *local_address_info;
  getaddrinfo(0, local_port, &hints, &local_address_info);


  logger("Creating socket...\n");
  SOCKET listen_connection_socket;
  listen_connection_socket = socket(local_address_info->ai_family, 
                                    local_address_info->ai_socktype,
                                    local_address_info->ai_protocol);
  if (!ISVALIDSOCKET(listen_connection_socket))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  logger("Binding socket to local address...\n");
  if (bind(listen_connection_socket, local_address_info->ai_addr, local_address_info->ai_addrlen))
  {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(local_address_info);


  logger("Listening...\n");
  if (listen(listen_connection_socket, 10) < 0)
  {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  fd_set masterset;
  FD_ZERO(&masterset);
  FD_SET(listen_connection_socket, &masterset);
  SOCKET max_socket = listen_connection_socket;

  fd_set channelset;
  FD_ZERO(&channelset);


  logger("Waiting for connections...\n");


  while (1)
  {
    fd_set readfds = masterset;

  
    if (select(max_socket + 1, &readfds, 0, 0, 0) < 0)
    {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }


    for (SOCKET i = 1; i <= max_socket; ++i)
    {
      if (FD_ISSET(i, &readfds))
      {
        if (i == listen_connection_socket)
        {
          /* Accept new pending connection */

          struct sockaddr_storage client_address;
          socklen_t client_len = sizeof(client_address);

          SOCKET socket_client = accept(listen_connection_socket,
                                        (struct sockaddr *)&client_address, &client_len);

          if (!ISVALIDSOCKET(socket_client))
          {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
          }

          FD_SET(socket_client, &masterset);
          FD_SET(socket_client, &channelset);

          max_socket = max(max_socket, socket_client);

          char address_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
          logger("New connection from %s\n", address_buffer);
        }
        else
        {
          /* Read message sent from client i */

          char read[4096];
          ssize_t bytes_received = recv(i, read, 4096, 0);
          if (bytes_received < 1)
          {
            FD_CLR(i, &masterset);
            FD_CLR(i, &channelset);
            CLOSESOCKET(i);
            continue;
          }

          read[bytes_received] = 0;

          logger("Retransmitting %d bytes from client %d: %s\n", bytes_received, i, read);

          transmit_message(i, read, (size_t)bytes_received, &channelset, max_socket + 1);
        }
      }
    }
  }

  logger("Closing listening socket...\n");
  CLOSESOCKET(listen_connection_socket);

  logger("Finished.\n");

  return 0;
}
