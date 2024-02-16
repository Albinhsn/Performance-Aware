#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG false

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
static inline char **getRegisterMemoryEncoding(bool w) {
  return w ? registerMemoryEncoding1 : registerMemoryEncoding0;
}

static inline char **getRegisterRegisterEncoding(bool w) {
  return w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;
}

static inline void advance(ui8 **buffer) { (*buffer)++; }

void parseRegMemoryFieldCoding(char *res, ui8 **buffer, ui8 mod, bool w,
                               ui8 rm) {
  char **encoding = getRegisterMemoryEncoding(mod == 0);
  switch (mod) {
  case 0: {
    if (rm == 6) {
      advance(buffer);
      ui16 direct = *buffer[0];
      if (w) {
        advance(buffer);
        direct = (*buffer[0] << 8) | direct;
      }
      sprintf(&res[0], "[%d]", direct);
    } else {
      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 1: {
    advance(buffer);
    if (*buffer[0] != 0) {
      sprintf(&res[0], "[%s + %d]", encoding[rm], *buffer[0]);
    } else {
      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 2: {
    advance(buffer);
    ui16 direct = *buffer[0];
    advance(buffer);
    direct = (*buffer[0] << 8) | direct;
    sprintf(&res[0], "[%s + %d]", encoding[rm], direct);
    break;
  }
  case 3: {
    sprintf(&res[0], "%s", getRegisterRegisterEncoding(w)[rm]);
    break;
  }
  }
}

static void parseImmediateToRegMemory(ui8 **buffer, ui8 mod, ui8 rm, bool w,
                                      bool s, bool mov) {

  char res[32];
  char source[32];
  parseRegMemoryFieldCoding(&res[0], buffer, mod, w, rm);

  advance(buffer);
  ui16 offset = *buffer[0];
  if (!s && w) {
    advance(buffer);
    offset = (*buffer[0] << 8) | offset;
    sprintf(&source[0], mov ? "word %d" : "%d", offset);
  } else {
    sprintf(&source[0], mov ? "byte %d" : "%d", offset);
  }

  printf(" %s, %s\n", res, source);
}
static void parseImmediateToReg(ui8 **buffer, ui8 immediate, ui8 reg, bool w) {
  if (w) {
    advance(buffer);
    ui16 immediate16 = (*buffer[0] << 8) | immediate;
    printf("mov %s, %d\n", registerToRegisterEncoding16[reg], immediate16);
  } else {
    printf("mov %s, %d\n", registerToRegisterEncoding8[reg], immediate);
  }
}
static void parseRegMemory(ui8 **buffer, char *instruction) {

  bool d = ((*buffer[0] >> 1) & 1);
  bool w = (*buffer[0] & 1);

  advance(buffer);
  ui8 mod = (*buffer[0] >> 6) & 0b11;
  ui8 reg = (*buffer[0] >> 3) & 0b111;
  ui8 rm = *buffer[0] & 0b111;

  char **encoding = getRegisterRegisterEncoding(w);
  char res[32];
  parseRegMemoryFieldCoding(&res[0], buffer, mod, w, rm);
  printf("%s %s, %s\n", instruction, d ? encoding[reg] : res,
         d ? res : encoding[reg]);
}

static void parseAccumulatorToMemory(ui8 **buffer, ui8 *current) {}

static inline void parseRegRegFieldEncoding(char *res, bool w, ui8 rm) {
  strcpy(&res[0], getRegisterRegisterEncoding(w)[w]);
}

static inline bool matchRegMemoryMove(ui8 current) {
  return (current >> 2 & 0b111111) == 0b100010;
}
static void parseImmediateToRegMove(ui8 **buffer) {
  bool w = (*buffer[0] >> 3 & 1);
  ui8 reg = (*buffer[0] & 0b111);

  advance(buffer);
  ui8 immediate = *buffer[0];

  printf("mov ");
  parseImmediateToReg(buffer, immediate, reg, w);
}

static void parseImmediateToAccumulator(ui8 **buffer, char *instruction) {

  bool w = *buffer[0] & 1;

  advance(buffer);
  ui16 imm = *buffer[0];
  if (w) {
    advance(buffer);
    imm = (*buffer[0] << 8) | imm;
  }

  printf("%s ax, %d\n", instruction, imm);
}

static void parseImmediateToRegMemoryAddSubCmp(ui8 **buffer) {
  bool s = ((*buffer[0] >> 1) & 1);
  bool w = (*buffer[0] & 1);
  advance(buffer);

  ui8 mod = (*buffer[0] >> 6) & 0b11;
  ui8 reg = (*buffer[0] >> 3) & 0b111;
  ui8 rm = *buffer[0] & 0b111;

  if (reg == 0b101) {
    printf("sub");
  } else if (reg == 0b111) {
    printf("cmp");
  } else {
    printf("add");
  }
  parseImmediateToRegMemory(buffer, mod, rm, w, s, false);
}
static void parseAccumulatorToMemoryMov(ui8 **buffer, bool mta) {

  bool w = *buffer[0] & 0b1;
  advance(buffer);
  ui16 offset = *buffer[0];
  advance(buffer);
  offset = (*buffer[0] << 8) | offset;
  if (!mta) {
    printf("mov [%d], ax\n", offset);
  } else {
    printf("mov ax, [%d]\n", offset);
  }
}

static void parseImmediateToRegMemoryMove(ui8 **buffer) {

  bool w = *buffer[0] & 0b1;
  advance(buffer);
  ui8 mod = (*buffer[0] >> 6) & 0b11;
  ui8 rm = *buffer[0] & 0b111;

  printf("mov");

  parseImmediateToRegMemory(buffer, mod, rm, w, 0, true);
}

static void parseJump(ui8 **buffer, char *instruction) {
  advance(buffer);
  printf("%s %d\n", instruction, *buffer[0]);
}
static inline bool matchImmediateToRegMemoryMove(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b1100011;
}

static inline bool matchImmediateToRegMove(ui8 current) {
  return ((current >> 4) & 0b1111) == 0b1011;
}

static inline bool matchImmediateToAccumulatorCmp(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b0011110;
}

static inline bool matchImmediateToAccumulatorSub(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b0010110;
}

static inline bool matchImmediateToAccumulatorAdd(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b0000010;
}

static inline bool matchMemoryToAccumulatorMov(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b1010000;
}

static inline bool matchAccumulatorToMemoryMov(ui8 current) {
  return ((current >> 1) & 0b1111111) == 0b1010001;
}

static inline bool matchRegMemoryCmp(ui8 current) {
  return ((current >> 2) & 0b111111) == 0b001110;
}

static inline bool matchRegMemorySub(ui8 current) {
  return ((current >> 2) & 0b111111) == 0b001010;
}

static inline bool matchRegMemoryAdd(ui8 current) {
  return ((current >> 2) & 0b111111) == 0;
}

static inline bool matchImmediateToRegMemory(ui8 current) {
  return ((current >> 2) & 0b111111) == 0b100000;
}

static inline bool matchJE(ui8 current) { return current == 0b01110100; }
static inline bool matchJL(ui8 current) { return current == 0b01111100; }
static inline bool matchJLE(ui8 current) { return current == 0b01111110; }
static inline bool matchJB(ui8 current) { return current == 0b01110010; }
static inline bool matchJBE(ui8 current) { return current == 0b01110110; }
static inline bool matchJP(ui8 current) { return current == 0b01111010; }
static inline bool matchJO(ui8 current) { return current == 0b01110000; }
static inline bool matchJS(ui8 current) { return current == 0b01111000; }
static inline bool matchJNE(ui8 current) { return current == 0b01110101; }
static inline bool matchJNL(ui8 current) { return current == 0b01111101; }
static inline bool matchJNLE(ui8 current) { return current == 0b01111111; }
static inline bool matchJNB(ui8 current) { return current == 0b01110011; }
static inline bool matchJNBE(ui8 current) { return current == 0b01110111; }
static inline bool matchJNP(ui8 current) { return current == 0b01111011; }
static inline bool matchJNO(ui8 current) { return current == 0b01110001; }
static inline bool matchJNS(ui8 current) { return current == 0b01111001; }
static inline bool matchLoop(ui8 current) { return current == 0b11100010; }
static inline bool matchLoopz(ui8 current) { return current == 0b11100001; }
static inline bool matchLoopnz(ui8 current) { return current == 0b11100000; }
static inline bool matchJCXZ(ui8 current) { return current == 0b11100011; }

int main() {
  ui8 *buffer;
  ui8 *end;
  int len;
  const char *name = "t1";
  read_file(&buffer, &len, name);

  end = buffer + len;
  printf("; %s.asm\n", name);
  printf("bits 16\n\n");

  char source[32];
  char dest[32];

  while (buffer < end) {
    if (matchRegMemoryAdd(buffer[0])) {
      parseRegMemory(&buffer, "add");

    } else if (matchRegMemoryCmp(buffer[0])) {
      parseRegMemory(&buffer, "cmp");

    } else if (matchRegMemorySub(buffer[0])) {
      parseRegMemory(&buffer, "sub");

    } else if (matchImmediateToRegMemory(buffer[0])) {
      parseImmediateToRegMemoryAddSubCmp(&buffer);

    } else if (matchImmediateToAccumulatorCmp(buffer[0])) {
      parseImmediateToAccumulator(&buffer, "cmp");

    } else if (matchImmediateToAccumulatorSub(buffer[0])) {
      parseImmediateToAccumulator(&buffer, "sub");

    } else if (matchImmediateToAccumulatorAdd(buffer[0])) {
      parseImmediateToAccumulator(&buffer, "add");

    } else if (matchImmediateToRegMove(buffer[0])) {
      parseImmediateToRegMove(&buffer);

    } else if (matchAccumulatorToMemoryMov(buffer[0])) {
      parseAccumulatorToMemoryMov(&buffer, false);

    } else if (matchMemoryToAccumulatorMov(buffer[0])) {
      parseAccumulatorToMemoryMov(&buffer, true);

    } else if (matchImmediateToRegMemoryMove(buffer[0])) {
      parseImmediateToRegMemoryMove(&buffer);

    } else if (matchRegMemoryMove(buffer[0])) {
      parseRegMemory(&buffer, "mov");

    } else if (matchJE(buffer[0])) {
      parseJump(&buffer, "je");

    } else if (matchJL(buffer[0])) {
      parseJump(&buffer, "jl");

    } else if (matchJLE(buffer[0])) {
      parseJump(&buffer, "jle");

    } else if (matchJB(buffer[0])) {
      parseJump(&buffer, "jb");

    } else if (matchJBE(buffer[0])) {
      parseJump(&buffer, "jbe");

    } else if (matchJP(buffer[0])) {
      parseJump(&buffer, "jp");

    } else if (matchJO(buffer[0])) {
      parseJump(&buffer, "jo");

    } else if (matchJS(buffer[0])) {
      parseJump(&buffer, "js");

    } else if (matchJNE(buffer[0])) {
      parseJump(&buffer, "jne");

    } else if (matchJNL(buffer[0])) {
      parseJump(&buffer, "jnl");

    } else if (matchJNLE(buffer[0])) {
      parseJump(&buffer, "jnle");

    } else if (matchJNB(buffer[0])) {
      parseJump(&buffer, "jnb");

    } else if (matchJNBE(buffer[0])) {
      parseJump(&buffer, "jnbe");

    } else if (matchJNP(buffer[0])) {
      parseJump(&buffer, "jp");

    } else if (matchJNO(buffer[0])) {
      parseJump(&buffer, "jno");

    } else if (matchJNS(buffer[0])) {
      parseJump(&buffer, "jns");

    } else if (matchLoop(buffer[0])) {
      parseJump(&buffer, "loop");

    } else if (matchLoopz(buffer[0])) {
      parseJump(&buffer, "loopz");

    } else if (matchLoopnz(buffer[0])) {
      parseJump(&buffer, "loopnz");

    } else if (matchJCXZ(buffer[0])) {
      parseJump(&buffer, "jcxz");

    } else {
      printf("UNKNOWN INSTRUCTION ");
      debugByte(buffer[0]);
      exit(1);
    }
    buffer++;
  }
}
