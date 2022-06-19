#include "tcp_socket.h"

int main(int argc, char *argv[])
{
  char *program_name = argv[0];


  if (argc < 3)
  {
    fprintf(stderr, "Usage: ./%s server_ip server_port\n", program_name);
    return 1;
  }


  char *server_ip = argv[1];
  char *server_port = argv[2];


  logger("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *server_address_info;
  if (getaddrinfo(server_ip, server_port, &hints, &server_address_info))
  {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  logger("Getting address info...\n");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(server_address_info->ai_addr, server_address_info->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
  logger("Remote address is: ");
  logger("%s %s\n", address_buffer, service_buffer);


  logger("Creating socket...\n");
  SOCKET server_socket;
  server_socket = socket(server_address_info->ai_family, server_address_info->ai_socktype,
                         server_address_info->ai_protocol);
  if (!ISVALIDSOCKET(server_socket))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  logger("Connecting...\n");
  if (connect(server_socket, server_address_info->ai_addr, server_address_info->ai_addrlen))
  {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(server_address_info);


  logger("Connected.\n");


  fd_set master;
  SOCKET max_socket = server_socket;
  FD_ZERO(&master);
  FD_SET(server_socket, &master);
  FD_SET(fileno(stdin), &master);


  printf("To send data, enter text followed by enter.\n");


  while (1)
  {
    fd_set readfds = master;


    if (select(max_socket + 1, &readfds, 0, 0, 0) < 0)
    {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }
  

    if (FD_ISSET(server_socket, &readfds))
    {
      char read[4096];
      int bytes_received = recv(server_socket, read, 4096, 0);
      if (bytes_received < 1)
      {
        logger("Connection closed by peer.\n");
        break;
      }
      printf("Received (%d bytes): %.*s\n",
             bytes_received, bytes_received, read);
    }

    if (FD_ISSET(fileno(stdin), &readfds))
    {
      char read[4096];
      if (!fgetstring(read, 4096, stdin))
        break;
      logger("Sending: %s\n", read);
      int bytes_sent = send(server_socket, read, strlen(read), 0);
      logger("Sent %d bytes.\n", bytes_sent);
    }
  }

  logger("Closing socket...\n");
  CLOSESOCKET(server_socket);

  logger("Finished.\n");
  return 0;
}
