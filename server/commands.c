#include <stdio.h>
#include <string.h>


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