#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/socket_connection.h"
#include "../shared/logger.h"
#include "utils.h"
#include "commands.h"
#include "connection.h"

#define MAX_USERS 1024 // Defines file descriptor interval to be used by users
#define MAX_CHANNELS 32

typedef struct
{
  char *name;
  SOCKET administrator;
  fd_set users;
} CHANNEL;

CHANNEL *find_channel(CHANNEL **channels, char *name)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL && strcmp(channels[i]->name, name) == 0)
    {
      return channels[i];
    }
  }
  return NULL;
}

CHANNEL *create_channel(CHANNEL **channels, char *name, SOCKET administrator)
{
  CHANNEL *channel = find_channel(channels, name);
  if (channel != NULL)
  {
    return channel;
  }
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] == NULL)
    {
      channels[i] = (CHANNEL *)malloc(sizeof(CHANNEL));
      channels[i]->name = name;
      channels[i]->administrator = administrator;
      FD_ZERO(&channels[i]->users);
      FD_SET(administrator, &channels[i]->users);
      return channels[i];
    }
  }
  return NULL;
}

void close_client_connection(SOCKET client_socket, fd_set *masterset, CHANNEL **channels)
{
  logger("Closing client connection...\n");
  CLOSESOCKET(client_socket);
  FD_CLR(client_socket, masterset);
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL)
    {
      FD_CLR(client_socket, &channels[i]->users);
      if (channels[i]->administrator == client_socket)
      {
        free(channels[i]->name);
        free(channels[i]);
        channels[i] = NULL;
      }
    }
  }
}

int add_client_to_channel(CHANNEL *channel, SOCKET client)
{
  if (channel == NULL)
  {
    return 1;
  }
  if (FD_ISSET(client, &channel->users))
  {
    return 0;
  }
  FD_SET(client, &channel->users);
  return 0;
}

void transmit_message(SOCKET message_sender, char *message_sender_nickname, char *message, size_t message_length, CHANNEL *channel, int nfds)
{
  char sender_boilerplate[128];
  int boilerplate_size;
  if (channel->administrator == message_sender)
  {
    boilerplate_size = sprintf(sender_boilerplate, "%s - (admin) %s: ", channel->name, message_sender_nickname);
  }
  else
  {
    boilerplate_size = sprintf(sender_boilerplate, "%s - %s: ", channel->name, message_sender_nickname);
  }

  for (int j = 0; j < nfds; j++)
  {
    if (FD_ISSET(j, &channel->users))
    {
      if (j != message_sender)
      {
        send(j, sender_boilerplate, boilerplate_size, 0);
        send(j, message, message_length, 0);
      }
    }
  }
}

void transmit_message_to_channels(SOCKET message_sender, char *message_sender_nickname, char *message, size_t message_length, CHANNEL **channels, int nfds)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL && FD_ISSET(message_sender, &channels[i]->users))
    {
      transmit_message(message_sender, message_sender_nickname, message, message_length, channels[i], nfds);
    }
  }
}

SOCKET find_client (char **nicknames, char *nickname)
{
  for (int i = 0; i < MAX_USERS; i++)
  {
    if (nicknames[i] != NULL && strcmp(nicknames[i], nickname) == 0)
    {
      return i;
    }
  }
  return -1;
}

int kick_user(SOCKET administrator, SOCKET client, CHANNEL **channels) {
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i] != NULL && channels[i]->administrator == administrator) {
      if (FD_ISSET(client, &channels[i]->users)) {
        FD_CLR(client, &channels[i]->users);
        return 0;
      }
    }
  }
  return 1;
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
  struct addrinfo hints = {0};
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

  char **nicknames = (char **)calloc(MAX_USERS, sizeof(char *));
  char *nickname;
  SOCKET user_fd;

  CHANNEL **channels = (CHANNEL **)calloc(MAX_CHANNELS, sizeof(CHANNEL *));
  CHANNEL *channel;
  char *channel_name;

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

          max_socket = max(max_socket, socket_client);

          char *client_nickname = (char *)calloc(50, sizeof(char));
          sprintf(client_nickname, "client_%d", socket_client);
          nicknames[socket_client] = client_nickname;

          char address_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
                      sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
          logger("New connection from %s. Using filedescriptor %d\n", address_buffer,
                 socket_client);

          send(socket_client, "Welcome to the IRC server!", strlen("Welcome to the IRC server!"), 0);
        }
        else
        {
          char read[4096];
          ssize_t bytes_received = recv(i, read, 4096, 0);
          if (bytes_received < 1)
          {
            close_client_connection(i, &masterset, channels);
            free(nicknames[i]);
            nicknames[i] = NULL;
            continue;
          }

          read[bytes_received] = 0;

          switch (identify_message_type(read))
          {
          case MESSAGE_TYPE_QUIT:
            logger("Removing client %d (%s) from channel.\n", i, nicknames[i]);
            close_client_connection(i, &masterset, channels);
            free(nicknames[i]);
            nicknames[i] = NULL;
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

          case MESSAGE_TYPE_JOIN:
            logger("Client %d requested to join channel: ", i);
            channel_name = (char *)calloc(50, sizeof(char));
            sscanf(read, "/join %s", channel_name);
            logger("%s\n", channel_name);
            channel = find_channel(channels, channel_name);
            if (channel == NULL)
            {
              logger("Creating channel %s.\n", channel_name);
              channel = create_channel(channels, channel_name, i);
            }
            else
            {
              add_client_to_channel(channel, i);
            }
            break;
          
          case MESSAGE_TYPE_KICK:
            logger("Client %d requested to kick client: ", i);
            nickname = (char *)calloc(50, sizeof(char));
            sscanf(read, "/kick %s", nickname);
            logger("%s\n", nickname);
            user_fd = find_client(nicknames, nickname);
            if (user_fd == -1)
            {
              send(i, "Server: Client not found!", strlen("Server: Client not found!"), 0);
            }
            else
            {
              kick_user(i, user_fd, channels);
            }
            break;

          case MESSAGE_TYPE_MUTE:

            break;

          case MESSAGE_TYPE_UNMUTE:

            break;

          case MESSAGE_TYPE_HELP:
            // TO BE IMPLEMENTED
            break;

          case MESSAGE_TYPE_RETRANSMISSION:
            logger("Retransmitting %d bytes from client %d (%s): %s\n", bytes_received, i, nicknames[i], read);
            transmit_message_to_channels(i, nicknames[i], read, bytes_received, channels, max_socket + 1);
            break;
          }
        }
      }
    }
  }

  logger("Closing listening socket...\n");
  CLOSESOCKET(listen_connection_socket);

  logger("Freeing variables...\n");
  for (int i = 0; i < MAX_USERS; i++)
  {
    if (nicknames[i])
      free(nicknames[i]);
  }
  free(nicknames);

  logger("Finished.\n");

  return 0;
}
