#ifndef STA_STRING_H
#define STA_STRING_H


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
struct String {
  uint64_t len;
  char *buffer;
};
typedef struct String String;

struct FileBuffer {
  uint64_t len;
  unsigned char *buffer;
};
typedef struct FileBuffer FileBuffer;

#endif
