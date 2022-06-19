#include "tcp_socket.h"

int main(int argc, char *argv[])
{
  char *program_name = argv[0];

  if (argc < 3)
  {
    fprintf(stderr, "Usage: ./%s hostname port\n", program_name);
    return 1;
  }

  char *hostname = argv[1];
  char *port = argv[2];

  logger("Configuring remote address...");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address))
  {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  logger("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr,
              peer_address->ai_addrlen,
              address_buffer,
              sizeof(address_buffer),
              service_buffer,
              sizeof(service_buffer),
              NI_NUMERICHOST);
  logger("%s %s\n", address_buffer, service_buffer);

  logger("Creating socket...");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family,
                       peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  logger("Connecting...\n");
  if (connect(socket_peer,
              peer_address->ai_addr,
              peer_address->ai_addrlen))
  {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(peer_address);

  logger("Connected.\n");
  logger("To send data, enter text followed by enter.\n");

  while (1)
  {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);
    FD_SET(fileno(stdin), &reads);

    if (select(socket_peer + 1, &reads, 0, 0, 0) < 0)
    {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }

    if (FD_ISSET(socket_peer, &reads))
    {
      char read[4096];
      int bytes_received = recv(socket_peer, read, 4096, 0);
      if (bytes_received < 1)
      {
        logger("Connection closed by peer.\n");
        break;
      }
      printf("Received (%d bytes): %.*s\n",
             bytes_received, bytes_received, read);
    }

    if (FD_ISSET(fileno(stdin), &reads))
    {
      char read[4096];
      if (!fgetstring(read, 4096, stdin))
        break;
      logger("Sending: %s\n", read);
      int bytes_sent = send(socket_peer, read, strlen(read), 0);
      logger("Sent %d bytes.\n", bytes_sent);
    }
  }

  logger("Closing socket...\n");
  CLOSESOCKET(socket_peer);

  logger("Finished.\n");
  return 0;
}
