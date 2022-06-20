#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>


#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)


#define MAX_USERS 1024
#define MAX_CHANNELS 32


typedef struct
{
  char *name;
  SOCKET administrator;
  fd_set users;
  fd_set muted_users;
} CHANNEL;
