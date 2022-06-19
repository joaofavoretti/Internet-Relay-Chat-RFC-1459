#include <stddef.h>
#include <stdio.h>

#include "../shared/socket_connection.h"
#include "connection.h"

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