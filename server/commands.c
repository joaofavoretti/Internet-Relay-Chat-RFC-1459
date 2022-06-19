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
    return MESSAGE_TYPE_NICKNAME;
  }
  else
  {
    return MESSAGE_TYPE_RETRANSMISSION;
  }
}