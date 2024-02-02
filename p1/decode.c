#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

char *registerEncoding16[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char *registerEncoding8[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

int main() {
  ui8 *buffer;
  int len;
  const char *name = "t2";
  read_file(&buffer, &len, name);

  printf("; %s.asm\n", name);
  printf("bits 16\n\n");

  for (; len > 0; len -= 2) {
    // debugByte(buffer[0]);
    if ((buffer[0] >> 2 & 0b111111) == 0b100010) {
      printf("mov ");
      bool d = ((buffer[0] >> 1) & 1) == 1;
      ui8 mod = (buffer[1] >> 6) & 0b11;
      ui8 reg = (buffer[1] >> 3) & 0b111;
      ui8 rm = buffer[1] & 0b111;

      char **encoding =
          (buffer[0] & 1) == 1 ? registerEncoding16 : registerEncoding8;

      printf("%s, %s\n", encoding[d ? reg : rm], encoding[d ? rm : reg]);
    } else {
      printf("WASN'T MOVE WAS %d\n", buffer[0] & 0b1111);
    }
    buffer += 2;
  }
}
