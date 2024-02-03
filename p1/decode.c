#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;

typedef float f32;
typedef double f64;

typedef int8_t i8;
typedef int16_t i16;
typedef int i32;
typedef int64_t i64;

bool read_file(unsigned char **buffer, int *len, const char *fileName) {
  FILE *filePtr;
  i64 fileSize, count;
  i32 error;

  filePtr = fopen(fileName, "rb");
  if (!filePtr) {
    return NULL;
  }

  fileSize = fseek(filePtr, 0, SEEK_END);
  fileSize = ftell(filePtr);

  *len = fileSize;
  *buffer = (unsigned char *)malloc(sizeof(unsigned char) * (fileSize + 1));
  (*buffer)[fileSize] = '\0';
  fseek(filePtr, 0, SEEK_SET);
  count = fread(*buffer, 1, fileSize, filePtr);
  if (count != fileSize) {
    free(*buffer);
    return false;
  }

  error = fclose(filePtr);
  if (error != 0) {
    free(*buffer);
    return false;
  }

  return true;
}

void debugByte(ui8 byte) {
  for (int i = 0; i < 8; i++) {
    printf("%d", byte >> i & 0b1);
  }
  printf("\n");
}

char *registerToRegisterEncoding16[] = {"ax", "cx", "dx", "bx",
                                        "sp", "bp", "si", "di"};
char *registerToRegisterEncoding8[] = {"al", "cl", "dl", "bl",
                                       "ah", "ch", "dh", "bh"};
char *registerMemoryEncoding0[] = {"bx + si", "bx + di", "bp + si", "bp + di",
                                   "si",      "di",      "",        "bx"};

char *registerMemoryEncoding1[] = {"bx + si", "bx + di", "bp + si", "bp + di",
                                   "si",      "di",      "bp",      "bx"};

static inline void advance(ui8 *current, ui8 **buffer) {
  *current = *buffer[0];
  (*buffer)++;
}

#define ADVANCE(len, buffer)                                                   \
  len++;                                                                       \
  buffer++

int main() {
  ui8 *buffer;
  ui8 *end;
  ui8 current;
  int len;
  const char *name = "t3";
  read_file(&buffer, &len, name);

  end = buffer + len;
  printf("; %s.asm\n", name);
  printf("bits 16\n\n");

  while (buffer < end) {
    advance(&current, &buffer);
    if (((current >> 4) & 0b1111) == 0b1011) {
      bool w = (current >> 3 & 1);
      ui8 reg = (current & 0b111);

      advance(&current, &buffer);
      ui8 immediate = current;

      if (w) {
        advance(&current, &buffer);
        ui16 immediate16 = (current << 8) | immediate;
        printf("mov %s, %d\n", registerToRegisterEncoding16[reg], immediate16);
      } else {
        printf("mov %s, %d\n", registerToRegisterEncoding8[reg], immediate);
      }

    } else if ((current >> 2 & 0b111111) == 0b100010) {
      bool d = ((current >> 1) & 1);
      bool w = (current & 1);

      advance(&current, &buffer);

      ui8 mod = (current >> 6) & 0b11;
      ui8 reg = (current >> 3) & 0b111;
      ui8 rm = current & 0b111;

      char source[32];
      memset(&source, 0, 16);
      char dest[32];
      memset(&dest, 0, 16);

      char **encoding =
          w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;

      // reg to reg
      if (mod == 0b00) {
        if (!d) {
          strcpy(&dest[0], encoding[reg]);
          sprintf(&source[0], "[%s]", registerMemoryEncoding0[rm]);
        } else {
          strcpy(&source[0], encoding[reg]);
          sprintf(&dest[0], "[%s]", registerMemoryEncoding0[rm]);
        }
      } else if (mod == 0b11) {
        strcpy(&source[0], encoding[d ? reg : rm]);
        strcpy(&dest[0], encoding[d ? rm : reg]);
      } else {

        advance(&current, &buffer);
        ui16 offset = current;

        if (mod == 0b10) {
          advance(&current, &buffer);
          offset = (current << 8) | offset;
        }

        char *location = registerMemoryEncoding1[rm];
        if (!d) {
          strcpy(&dest[0], encoding[reg]);
          if (offset != 0) {
            sprintf(&source[0], "[%s + %d]", location, offset);
          } else {
            sprintf(&source[0], "[%s]", location);
          }

        } else {
          strcpy(&source[0], encoding[reg]);
          if (offset != 0) {
            sprintf(&dest[0], "[%s + %d]", location, offset);
          } else {
            sprintf(&dest[0], "[%s]", location);
          }
        }
      }

      printf("mov %s, %s\n", source, dest);

    } else {
      printf("UNKNOWN INSTRUCTION ");
      debugByte(current);
    }
  }
}
