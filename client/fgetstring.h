#include <stdio.h>

int fgetstring(char *buffer, int size, FILE *stream)
{
  int count = 0;
  int c;

  while (count < size - 1 && (c = fgetc(stream)) != EOF && c != '\n')
  {
    buffer[count++] = (char)c;
  }

  buffer[count] = '\0';

  return count;
}
