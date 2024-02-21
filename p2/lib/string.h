#ifndef STA_STRING_H
#define STA_STRING_H


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
struct String {
  u64 len;
  u8 *buffer;
};
typedef struct String String;

#endif
