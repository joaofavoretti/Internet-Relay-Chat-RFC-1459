#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#include "../shared/socket_connection.h"
#include "../shared/logger.h"
#include "connection.h"


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
      FD_CLR(client_socket, &channels[i]->muted_users);
      if (channels[i]->administrator == client_socket)
      {
        free(channels[i]->name);
        free(channels[i]);
        channels[i] = NULL;
      }
    }
  }
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
    if (channels[i] != NULL && FD_ISSET(message_sender, &channels[i]->users) && !FD_ISSET(message_sender, &channels[i]->muted_users))
    {
      transmit_message(message_sender, message_sender_nickname, message, message_length, channels[i], nfds);
    }
  }
}
