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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void logger(const char *format, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
#endif
}

int fgetstring(char *buffer, int size, FILE *stream) {
    int count = 0;
    int c;

    while (count < size - 1 && (c = fgetc(stream)) != EOF && c != '\n') {
        buffer[count++] = (char) c;
    }

    buffer[count] = '\0';

    return count;
}
