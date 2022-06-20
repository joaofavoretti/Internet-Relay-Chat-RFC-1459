#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/socket_connection.h"
#include "../shared/logger.h"
#include "utils.h"
#include "commands.h"
#include "connection.h"


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

  char *help_message;

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
            free(nickname);
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
            logger("Client %d requested to mute client: ", i);
            nickname = (char *)calloc(50, sizeof(char));
            sscanf(read, "/mute %s", nickname);
            logger("%s\n", nickname);
            user_fd = find_client(nicknames, nickname);
            free(nickname);
            if (user_fd == -1)
            {
              send(i, "Server: Client not found!", strlen("Server: Client not found!"), 0);
            }
            else
            {
              mute_user(i, user_fd, channels);
            }

            break;

          case MESSAGE_TYPE_UNMUTE:
            logger("Client %d requested to unmute client: ", i);
            nickname = (char *)calloc(50, sizeof(char));
            sscanf(read, "/unmute %s", nickname);
            logger("%s\n", nickname);
            user_fd = find_client(nicknames, nickname);
            free(nickname);
            if (user_fd == -1)
            {
              send(i, "Server: Client not found!", strlen("Server: Client not found!"), 0);
            }
            else
            {
              unmute_user(i, user_fd, channels);
            }

            break;

          case MESSAGE_TYPE_WHOIS:
            logger("Client %d requested to whois client: ", i);
            nickname = (char *)calloc(50, sizeof(char));
            sscanf(read, "/whois %s", nickname);
            logger("%s\n", nickname);
            user_fd = find_client(nicknames, nickname);
            free(nickname);
            if (user_fd == -1)
            {
              send(i, "Server: Client not found!", strlen("Server: Client not found!"), 0);
            }
            else
            {
              whois_user(i, user_fd, nicknames);
            }
            break;

          case MESSAGE_TYPE_HELP:
            logger("Client %d requested help.\n", i);
            help_message = (char *)calloc(4096, sizeof(char));
            sprintf(help_message, "Server commands:\n");
            strcat(help_message, "/nickname <nickname> - change nickname\n");
            strcat(help_message, "/join <channel> - join channel\n");
            strcat(help_message, "/kick <nickname> - kick user from channel\n");
            strcat(help_message, "/mute <nickname> - mute user\n");
            strcat(help_message, "/unmute <nickname> - unmute user\n");
            strcat(help_message, "/whois <nickname> - show user information\n");
            strcat(help_message, "/help - show this help message\n");
            strcat(help_message, "/quit - quit server\n\n");
            send(i, help_message, strlen(help_message), 0);
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
