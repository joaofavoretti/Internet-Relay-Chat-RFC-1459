#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "../shared/socket_connection.h"
#include "commands.h"


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
  else if (strncmp(message, "/join", strlen("/join")) == 0)
  {
    return MESSAGE_TYPE_JOIN;
  }
  else if (strncmp(message, "/kick", strlen("/kick")) == 0)
  {
    return MESSAGE_TYPE_KICK;
  }
  else if (strncmp(message, "/mute", strlen("/mute")) == 0)
  {
    return MESSAGE_TYPE_MUTE;
  }
  else if (strncmp(message, "/unmute", strlen("/unmute")) == 0)
  {
    return MESSAGE_TYPE_UNMUTE;
  }
  else if (strncmp(message, "/whois", strlen("/whois")) == 0)
  {
    return MESSAGE_TYPE_WHOIS;
  }
  else if (strncmp(message, "/", strlen("/")) == 0)
  {
    return MESSAGE_TYPE_HELP;
  }
  else
  {
    return MESSAGE_TYPE_RETRANSMISSION;
  }
}

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
      FD_ZERO(&channels[i]->muted_users);
      FD_SET(administrator, &channels[i]->users);
      return channels[i];
    }
  }
  return NULL;
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

SOCKET find_client(char **nicknames, char *nickname)
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

int kick_user(SOCKET administrator, SOCKET client, CHANNEL **channels)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL && channels[i]->administrator == administrator)
    {
      if (FD_ISSET(client, &channels[i]->users))
      {
        FD_CLR(client, &channels[i]->users);
        return 0;
      }
    }
  }
  return 1;
}

int mute_user(SOCKET administrator, SOCKET client, CHANNEL **channels)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL && channels[i]->administrator == administrator)
    {
      if (FD_ISSET(client, &channels[i]->users))
      {
        FD_SET(client, &channels[i]->muted_users);
        return 0;
      }
    }
  }
  return 1;
}

int unmute_user(SOCKET administrator, SOCKET client, CHANNEL **channels)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    if (channels[i] != NULL && channels[i]->administrator == administrator)
    {
      if (FD_ISSET(client, &channels[i]->muted_users))
      {
        FD_CLR(client, &channels[i]->muted_users);
        return 0;
      }
    }
  }
  return 1;
}

int whois_user(SOCKET administrator, SOCKET client, char **nicknames)
{
  struct sockaddr_in client_address;
  socklen_t client_address_length = sizeof(client_address);
  if (getpeername(client, (struct sockaddr *)&client_address, &client_address_length) < 0)
  {
    return 1;
  }
  char ip_address[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_address.sin_addr, ip_address, INET_ADDRSTRLEN);

  char *nickname = nicknames[client];

  char whois_message[256];
  sprintf(whois_message, "Server: %s is connected from %s", nickname, ip_address);
  send(administrator, whois_message, strlen(whois_message), 0);

  return 0;
}
