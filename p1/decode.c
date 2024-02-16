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

static inline void advance(ui8 *current, ui8 **buffer) {
  *current = *buffer[0];
  (*buffer)++;
}

void parseRegMemoryFieldCoding(char *res, ui8 **buffer, ui8 *current, ui8 mod,
                               bool w, ui8 rm) {
  char **encoding =
      mod == 0 ? registerMemoryEncoding0 : registerMemoryEncoding1;
  switch (mod) {
  case 0: {
    if (rm == 6) {
      advance(current, buffer);
      ui16 direct = *current;
      if (w) {
        advance(current, buffer);
        direct = (*current << 8) | direct;
      }
      sprintf(&res[0], "[%d]", direct);
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
    advance(current, buffer);
    ui16 direct = *current;
    advance(current, buffer);
    direct = (*current << 8) | direct;
    sprintf(&res[0], "[%s + %d]", encoding[rm], direct);
    break;
  }
  case 3: {
    char **enc = w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;
    sprintf(&res[0], "%s", enc[rm]);
    break;
  }
  }
}

static void parseImmediateToRegMemory(ui8 **buffer, ui8 *current, ui8 mod,
                                      ui8 rm, bool w, bool s, bool mov) {

  char res[32];
  char source[32];
  parseRegMemoryFieldCoding(&res[0], buffer, current, mod, w, rm);

  advance(current, buffer);
  ui16 offset = *current;
  if (!s && w) {
    advance(current, buffer);
    offset = (*current << 8) | offset;
    sprintf(&source[0], mov ? "word %d" : "%d", offset);
  } else {
    sprintf(&source[0], mov ? "byte %d" : "%d", offset);
  }

  printf(" %s, %s\n", res, source);
}
static void parseImmediateToReg(ui8 **buffer, ui8 *current, ui8 immediate,
                                ui8 reg, bool w) {
  if (w) {
    advance(current, buffer);
    ui16 immediate16 = (*current << 8) | immediate;
    printf("mov %s, %d\n", registerToRegisterEncoding16[reg], immediate16);
  } else {
    printf("mov %s, %d\n", registerToRegisterEncoding8[reg], immediate);
  }
}
static void parseRegMemory(ui8 **buffer, ui8 *current, char *instruction) {

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
  printf("%s %s, %s\n", instruction, d ? encoding[reg] : res,
         d ? res : encoding[reg]);
}

static void parseAccumulatorToMemory(ui8 **buffer, ui8 *current) {}

void parseRegRegFieldEncoding(char *res, bool w, ui8 rm) {
  char *enc =
      w ? registerToRegisterEncoding16[rm] : registerToRegisterEncoding8[rm];
  strcpy(&res[0], enc);
}

static inline bool matchRegMemoryMove(ui8 current) {
  return (current >> 2 & 0b111111) == 0b100010;
}
static void parseImmediateToRegMove(ui8 **buffer, ui8 *current) {
  bool w = (*current >> 3 & 1);
  ui8 reg = (*current & 0b111);

  advance(current, buffer);
  ui8 immediate = *current;

  printf("mov ");
  parseImmediateToReg(buffer, current, immediate, reg, w);
}

static void parseImmediateToAccumulator(ui8 **buffer, ui8 *current,
                                        char *instruction) {
#if DEBUG
  printf("ADD Immediate to accumulator\n\t");
#endif

  bool w = *current & 1;

  advance(current, buffer);
  ui16 imm = *current;
  if (w) {
    advance(current, buffer);
    imm = (*current << 8) | imm;
  }

  printf("%s ax, %d\n", instruction, imm);
}

static void parseImmediateToRegMemoryAddSubCmp(ui8 **buffer, ui8 *current) {
#if DEBUG
  printf("ADD Immediate to register/memory\n\t");
#endif
  bool s = ((*current >> 1) & 1);
  bool w = (*current & 1);
  advance(current, buffer);

  ui8 mod = (*current >> 6) & 0b11;
  ui8 reg = (*current >> 3) & 0b111;
  ui8 rm = *current & 0b111;

  if (reg == 0b101) {
    printf("sub");
  } else if (reg == 0b111) {
    printf("cmp");
  } else {
    printf("add");
  }
  parseImmediateToRegMemory(buffer, current, mod, rm, w, s, false);
}
static void parseAccumulatorToMemoryMov(ui8 **buffer, ui8 *current, bool mta) {

  bool w = *current & 0b1;
  advance(current, buffer);
  ui16 offset = *current;
  advance(current, buffer);
  offset = (*current << 8) | offset;
  if (!mta) {
    printf("mov [%d], ax\n", offset);
  } else {
    printf("mov ax, [%d]\n", offset);
  }
}

static void parseImmediateToRegMemoryMove(ui8 **buffer, ui8 *current) {

  bool w = *current & 0b1;
  advance(current, buffer);
  ui8 mod = (*current >> 6) & 0b11;
  ui8 rm = *current & 0b111;

  parseImmediateToRegMemory(buffer, current, mod, rm, w, 0, true);
}

static void parseJump(ui8 **buffer, ui8 *current, char *instruction) {
  advance(current, buffer);
  printf("%s %d\n", instruction, *current);
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
  ui8 current;
  int len;
  const char *name = "t5_test";
  read_file(&buffer, &len, name);

  end = buffer + len;
  printf("; %s.asm\n", name);
  printf("bits 16\n\n");

  char source[32];
  char dest[32];

  while (buffer < end) {
    advance(&current, &buffer);
    if (matchRegMemoryAdd(current)) {
      parseRegMemory(&buffer, &current, "add");

    } else if (matchRegMemoryCmp(current)) {
      parseRegMemory(&buffer, &current, "cmp");

    } else if (matchRegMemorySub(current)) {
      parseRegMemory(&buffer, &current, "sub");

    } else if (matchImmediateToRegMemory(current)) {
      parseImmediateToRegMemoryAddSubCmp(&buffer, &current);

    } else if (matchImmediateToAccumulatorCmp(current)) {
      parseImmediateToAccumulator(&buffer, &current, "cmp");

    } else if (matchImmediateToAccumulatorSub(current)) {
      parseImmediateToAccumulator(&buffer, &current, "sub");

    } else if (matchImmediateToAccumulatorAdd(current)) {
      parseImmediateToAccumulator(&buffer, &current, "add");

    } else if (matchImmediateToRegMove(current)) {
      parseImmediateToRegMove(&buffer, &current);

    } else if (matchAccumulatorToMemoryMov(current)) {
      parseAccumulatorToMemoryMov(&buffer, &current, false);

    } else if (matchMemoryToAccumulatorMov(current)) {
      parseAccumulatorToMemoryMov(&buffer, &current, true);

    } else if (matchImmediateToRegMemoryMove(current)) {
      parseImmediateToRegMemoryMove(&buffer, &current);

    } else if (matchRegMemoryMove(current)) {
      parseRegMemory(&buffer, &current, "mov");

    } else if (matchJE(current)) {
      parseJump(&buffer, &current, "je");

    } else if (matchJL(current)) {
      parseJump(&buffer, &current, "jl");

    } else if (matchJLE(current)) {
      parseJump(&buffer, &current, "jle");

    } else if (matchJB(current)) {
      parseJump(&buffer, &current, "jb");

    } else if (matchJBE(current)) {
      parseJump(&buffer, &current, "jbe");

    } else if (matchJP(current)) {
      parseJump(&buffer, &current, "jp");

    } else if (matchJO(current)) {
      parseJump(&buffer, &current, "jo");

    } else if (matchJS(current)) {
      parseJump(&buffer, &current, "js");

    } else if (matchJNE(current)) {
      parseJump(&buffer, &current, "jne");

    } else if (matchJNL(current)) {
      parseJump(&buffer, &current, "jnl");

    } else if (matchJNLE(current)) {
      parseJump(&buffer, &current, "jnle");

    } else if (matchJNB(current)) {
      parseJump(&buffer, &current, "jnb");

    } else if (matchJNBE(current)) {
      parseJump(&buffer, &current, "jnbe");

    } else if (matchJNP(current)) {
      parseJump(&buffer, &current, "jp");

    } else if (matchJNO(current)) {
      parseJump(&buffer, &current, "jno");

    } else if (matchJNS(current)) {
      parseJump(&buffer, &current, "jns");

    } else if (matchLoop(current)) {
      parseJump(&buffer, &current, "loop");

    } else if (matchLoopz(current)) {
      parseJump(&buffer, &current, "loopz");

    } else if (matchLoopnz(current)) {
      parseJump(&buffer, &current, "loopnz");

    } else if (matchJCXZ(current)) {
      parseJump(&buffer, &current, "jcxz");

    } else {
      printf("UNKNOWN INSTRUCTION ");
      debugByte(current);
    }
  }
}
