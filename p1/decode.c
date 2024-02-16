#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG true

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
  for (int i = 7; i >= 0; i--) {
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
static void parseImmediateToRegMemory(ui8 **buffer, ui8 *current) {}
static void parseImmediateToReg(ui8 **buffer, ui8 *current) {}
static void parseRegMemoryAdd(ui8 **buffer, ui8 *current);
static void parseAccumulatorToMemory(ui8 **buffer, ui8 *current) {}

void parseRegRegFieldEncoding(char *res, bool w, ui8 rm) {
  char *enc =
      w ? registerToRegisterEncoding16[rm] : registerToRegisterEncoding8[rm];
  strcpy(&res[0], enc);
}

void parseRegMemoryFieldCoding(char *res, ui8 **buffer, ui8 *current, ui8 mod,
                               bool w, ui8 rm) {
  char **encoding = w ? registerMemoryEncoding1 : registerMemoryEncoding0;
  switch (mod) {
  case 0: {
    if (rm == 6) {
      ui16 direct = *current;
      if (w) {
        advance(current, buffer);
        direct = (*current << 8) | direct;
      }
      sprintf(&res[0], "%d", direct);
    } else {
      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 1: {
    advance(current, buffer);
    if (*current != 0) {
      sprintf(&res[0], "[%s + %d]", encoding[rm], *current);
    } else {
      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 2: {
    ui16 direct = *current;
    advance(current, buffer);
    direct = (*current << 8) | direct;
    sprintf(&res[0], "[%s + %d]", encoding[rm], direct);
    break;
  }
  case 3: {
    sprintf(&res[0], "%s", encoding[rm]);
    break;
  }
  }
}

static inline bool matchRegMemoryMove(ui8 current) {
  return (current >> 2 & 0b111111) == 0b100010;
}
static void parseImmediateToRegMove(ui8 **buffer, ui8 *current) {
  bool w = (*current >> 3 & 1);
  ui8 reg = (*current & 0b111);

  advance(current, buffer);
  ui8 immediate = *current;

  if (w) {
    advance(current, buffer);
    ui16 immediate16 = (*current << 8) | immediate;
    printf("mov %s, %d\n", registerToRegisterEncoding16[reg], immediate16);
  } else {
    printf("mov %s, %d\n", registerToRegisterEncoding8[reg], immediate);
  }
}

static void parseImmediateToRegMemoryAdd(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("ADD Immediate to register/memory\n\t");
#endif
  bool s = ((*current >> 1) & 1);
  bool w = (*current & 1);
  advance(current, buffer);

  ui8 mod = (*current >> 6) & 0b11;
  ui8 rm = *current & 0b111;
  advance(current, buffer);

  char **encoding =
      w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;
  printf("add %s, ", encoding[rm]);
  ui16 offset = *current;
}
static void parseAccumulatorToMemoryMov(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("Accumulator to memory mov\n\t");
#endif
  char source[32];
  memset(&source, 0, 16);
  char dest[32];
  memset(&dest, 0, 16);

  bool w = *current & 0b1;
  advance(current, buffer);
  ui16 offset = *current;
  advance(current, buffer);
  offset = (*current << 8) | offset;
  if (w) {
    printf("mov [%d], ax\n", offset);
  } else {
    printf("mov ax, [%d]\n", offset);
  }
}

static void parseImmediateToRegMemoryMove(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("Accumulator to memory mov\n\t");
#endif
  char source[32];
  char dest[32];

  memset(&source, 0, 32);
  memset(&dest, 0, 32);

  bool w = *current & 0b1;
  advance(current, buffer);
  ui8 mod = (*current >> 6) & 0b11;
  ui8 rm = *current & 0b111;
  char res[32];
  parseRegMemoryFieldCoding(&res[0], buffer, current, mod, w, rm);

  advance(current, buffer);
  ui16 offset = *current;
  if (w) {
    advance(current, buffer);
    offset = (*current << 8) | offset;
    sprintf(&source[0], "word %d", offset);
  } else {
    sprintf(&source[0], "byte %d", offset);
  }

  printf("mov %s, %s\n", dest, source);
}
static void parseRegMemoryAdd(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("ADD Reg/memory with register to either\n\t");
#endif
  bool d = ((*current >> 1) & 1);
  bool w = (*current & 1);

  advance(current, buffer);
  ui8 mod = (*current >> 6) & 0b11;
  ui8 reg = (*current >> 3) & 0b111;
  ui8 rm = *current & 0b111;

  char **encoding =
      w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;
  char res[32];
  parseRegMemoryFieldCoding(&res[0], buffer, current, mod, w, rm);
  printf("add %s, %s\n", encoding[reg], res);
}

static void parseRegMemoryMove(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("Reg Memory Move\n\t");
#endif
  char source[32];
  char dest[32];
  memset(&source, 0, 32);
  memset(&dest, 0, 32);

  bool d = ((*current >> 1) & 1);
  bool w = (*current & 1);

  advance(current, buffer);
  ui8 mod = (*current >> 6) & 0b11;
  ui8 reg = (*current >> 3) & 0b111;
  ui8 rm = *current & 0b111;

  if (mod == 0b11) {
    ui8 fst = d ? reg : rm;
    ui8 snd = d ? rm : reg;
    parseRegRegFieldEncoding(&dest[0], w, fst);
    parseRegRegFieldEncoding(&source[0], w, snd);
  } else {

    char **encoding =
        w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;

    advance(current, buffer);
    ui16 offset = *current;

    if (mod == 0b10) {
      advance(current, buffer);
      offset = (*current << 8) | offset;
    }

    strcpy(d ? &dest[0] : &source[0], encoding[reg]);
    char *location = registerMemoryEncoding1[rm];
    if (offset != 0) {
      sprintf(d ? &source[0] : &dest[0], "[%s + %d]", location, offset);
    } else {
      sprintf(d ? &source[0] : &dest[0], "[%s]", location);
    }
  }

  printf("mov %s, %s\n", dest, source);
}
static inline bool matchImmediateToRegMemoryMove(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b1100011;
}

static inline bool matchImmediateToRegMove(ui8 current) {
  return ((current >> 4) & 0b1111) == 0b1011;
}

static inline bool matchImmediateToAccumulatorAdd(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b0000010;
}

static inline bool matchAccumulatorToMemoryMov(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b1010001;
}

static inline bool matchRegMemoryAdd(ui8 current) {
  return ((current >> 2) & 0b111111) == 0;
}

static inline bool matchImmediateToRegMemoryAdd(ui8 current) {
  return ((current >> 2) & 0b111111) == 0b100000;
}

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

  char source[32];
  char dest[32];

  while (buffer < end) {
    advance(&current, &buffer);
    if (matchRegMemoryAdd(current)) {
      parseRegMemoryAdd(&buffer, &current);
    } else if (matchImmediateToRegMemoryAdd(current)) {
      parseImmediateToRegMemoryAdd(&buffer, &current);
    } else if (matchImmediateToAccumulatorAdd(current)) {
    } else if (matchImmediateToRegMove(current)) {
      parseImmediateToRegMove(&buffer, &current);
    } else if (matchAccumulatorToMemoryMov(current)) {
      parseAccumulatorToMemoryMov(&buffer, &current);
    } else if (matchImmediateToRegMemoryMove(current)) {
      parseImmediateToRegMemoryMove(&buffer, &current);
    } else if (matchRegMemoryMove(current)) {
      parseRegMemoryMove(&buffer, &current);
    } else {
      // printf("UNKNOWN INSTRUCTION ");
      // debugByte(current);
    }
  }
}
