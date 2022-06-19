#include "../shared/socket_connection.h"
#include "../shared/logger.h"


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_NICKNAMES 1024


enum MESSAGE_TYPE {
  MESSAGE_TYPE_QUIT = 0x1,
  MESSAGE_TYPE_PING = 0x2,
  MESSAGE_TYPE_NICKNAME = 0x4,
  MESSAGE_TYPE_RETRANSMISSION = 0x8,
};

int identify_message_type(char *message)
{
  if (strncmp(message, "/quit", strlen("/quit")) == 0)
  {
    return MESSAGE_TYPE_QUIT;
  }
  else if (strncmp(message, "/ping", strlen("/ping")) == 0)
  {
    return MESSAGE_TYPE_PING;
  }
  else if (strncmp(message, "/nickname", strlen("/nickname")) == 0)
  {
    return MESSAGE_TYPE_NICKNAME;
  }
  else
  {
    return MESSAGE_TYPE_RETRANSMISSION;
  }
}

int max(int a, int b)
{
  return a > b ? a : b;
}

void close_client_connecion(SOCKET client_socket, fd_set *masterset, fd_set *channelset)
{
  if (ISVALIDSOCKET(client_socket))
  {
    FD_CLR(client_socket, masterset);
    FD_CLR(client_socket, channelset);
    CLOSESOCKET(client_socket);
  }
}

void transmit_message(SOCKET message_sender, char *message_sender_nickname, char *message, size_t message_length, fd_set *channelset, int nfds) {
  char sender_boilerplate[128];
  int boilerplate_size = sprintf(sender_boilerplate, "%s: ", message_sender_nickname);
  
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
  listen_connection_socket = socket(local_address_info->ai_family, local_address_info->ai_socktype,
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
  /* TODO: Talvez passar isso pra uma linked list :/ */
  char **nicknames = (char **)calloc(MAX_NICKNAMES, sizeof(char *));


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

          char *client_nickname = (char *)calloc(50, sizeof(char));
          sprintf(client_nickname, "Client %d", socket_client);
          nicknames[socket_client] = client_nickname;

          char address_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
                      sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
          logger("New connection from %s. Using filedescriptor %d\n", address_buffer,
                 socket_client);

          send(socket_client, "Welcome to the channel!", strlen("Welcome to the channel!"), 0);
        }
        else
        {
          char read[4096];
          ssize_t bytes_received = recv(i, read, 4096, 0);
          if (bytes_received < 1)
          {
            close_client_connecion(i, &masterset, &channelset);
            free(nicknames[i]);
            nicknames[i] = 0;
            continue;
          }

          read[bytes_received] = 0;

          switch(identify_message_type(read))
          {
            case MESSAGE_TYPE_QUIT:
              logger("Removing client %d (%s) from channel.\n", i, nicknames[i]);
              close_client_connecion(i, &masterset, &channelset);
              free(nicknames[i]);
              nicknames[i] = 0;
              break;

            case MESSAGE_TYPE_PING:
              logger("Sending PONG to client %d (%s).\n", i, nicknames[i]);
              send(i, "Server: PONG!", 13, 0);
              break;

            case MESSAGE_TYPE_NICKNAME:
              logger("Changing nickname of client %d (%s) to: ", i, nicknames[i]);
              sscanf(read, "/nickname %s", nicknames[i]);
              logger("%s\n", nicknames[i]);
              break;

            case MESSAGE_TYPE_RETRANSMISSION:
              logger("Retransmitting %d bytes from client %d (%s): %s\n", bytes_received, i, nicknames[i], read);
              transmit_message(i, nicknames[i], read, bytes_received, &channelset, max_socket + 1);
              break;
          }
        }
      }
    }
  }

  logger("Closing listening socket...\n");
  CLOSESOCKET(listen_connection_socket);

  logger("Freeing variables...\n");
  for (int i = 0; i < MAX_NICKNAMES; i++)
  {
    if (nicknames[i])
      free(nicknames[i]);
  }
  free(nicknames);

  logger("Finished.\n");

  return 0;
}
