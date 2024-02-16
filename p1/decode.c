#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;

typedef float    f32;
typedef double   f64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int      i32;
typedef int64_t  i64;

char*            registerToRegisterEncoding16[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char*            registerToRegisterEncoding8[]  = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char*            registerMemoryEncoding0[]      = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "", "bx"};
char*            registerMemoryEncoding1[]      = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

enum Operation
{
  MOV,
  ADD,
  SUB,
  CMP,
  JE,
  JL,
  JLE,
  JB,
  JBE,
  JP,
  JO,
  JS,
  JNE,
  JNL,
  JNLE,
  JNB,
  JNBE,
  JNP,
  JNO,
  JNS,
  LOOP,
  LOOPZ,
  LOOPNZ,
  JCXZ
};
typedef enum Operation Operation;

enum ImmediateSize
{
  EIGHT,
  SIXTEEN,
  ZERO
};
typedef enum ImmediateSize ImmediateSize;

enum OperandType
{
  ACCUMULATOR,
  REGISTER,
  IMMEDIATE,
  EFFECTIVEADDRESS
};
typedef enum OperandType OperandType;

struct Immediate
{
  ImmediateSize size;
  union
  {
    ui16 immediate16;
    ui8  immediate8;
  };
};
typedef struct Immediate Immediate;

struct EffectiveAddress
{
  ui8       mod;
  ui8       rm;
  Immediate immediate;
};
typedef struct EffectiveAddress EffectiveAddress;

char*                           registerNames[] = {"A", "C", "D", "B", "SP", "BP", "SI", "DI"};

enum RegisterType
{
  A,
  C,
  D,
  B,
  SP,
  BP,
  SI,
  DI
};
typedef enum RegisterType RegisterType;

struct Register
{
  RegisterType type;
  ui8          offset;
  ui16         size;
};
typedef struct Register Register;

struct Operand
{
  OperandType type;
  union
  {
    Immediate        immediate;
    Register         reg;
    EffectiveAddress effectiveAddress;
  };
};
typedef struct Operand Operand;

struct Instruction
{
  Operation op;
  Operand   operands[2];
};
typedef struct Instruction Instruction;

RegisterType               registerEncoded[8] = {A, C, D, B, SP, BP, SI, DI};

void                       parseRegister(Register* regis, ui8 reg, bool w)
{
  if (w)
  {
    regis->offset = 0;
    regis->size   = 16;
    regis->type   = registerEncoded[reg];
  }
  else if (reg <= 3)
  {
    regis->offset = 0;
    regis->size   = 8;
    regis->type   = registerEncoded[reg];
  }
  else
  {
    regis->type   = registerEncoded[reg - 4];
    regis->size   = 8;
    regis->offset = 8;
  }
}
const char* opToString[]  = {"mov", "add", "sub", "cmp", "je", "jl", "jle", "jb", "jbe", "jp", "jo", "js", "jne", "jnl", "jnle", "jnb", "jnbe", "jnp", "jno", "jns", "loop", "loopz", "loopnz", "jcxz"};

const char* regToString[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char* regToStringHigh[] = {"ah", "ch", "dh", "bh"};
const char* regToStringLow[]  = {"al", "cl", "dl", "bl"};

void        debugRegister(Register reg)
{
  if (reg.size == 16)
  {
    printf("%s", regToString[reg.type]);
  }
  else if (reg.offset == 8)
  {
    printf("%s", regToStringHigh[reg.type]);
  }
  else
  {
    printf("%s", regToStringLow[reg.type]);
  }
}

void debugEffectiveAddress(EffectiveAddress effectiveAddress)
{
  printf("[");
  if (effectiveAddress.mod == 0 && effectiveAddress.rm == 6)
  {
    printf("%d", effectiveAddress.immediate.size == 8 ? effectiveAddress.immediate.immediate8 : effectiveAddress.immediate.immediate16);
  }
  else
  {
    printf("%s", registerMemoryEncoding1[effectiveAddress.rm]);
    if (effectiveAddress.immediate.size == ZERO)
    {
    }
    else if (effectiveAddress.mod == 1)
    {
      printf(" + %d", effectiveAddress.immediate.immediate8);
    }
    else
    {
      printf(" + %d", effectiveAddress.immediate.immediate16);
    }
  }
  printf("]");
}
void debugOperand(Operand operand)
{
  switch (operand.type)
  {
  case EFFECTIVEADDRESS:
  {
    debugEffectiveAddress(operand.effectiveAddress);
    break;
  }
  case REGISTER:
  {
    debugRegister(operand.reg);
    break;
  }
  case ACCUMULATOR:
  {
    printf("ax");
    break;
  }
  case IMMEDIATE:
  {
    printf("%d", operand.immediate.size == EIGHT ? operand.immediate.immediate8 : operand.immediate.immediate16);
  }
  }
}

void debugInstruction(Instruction instruction)
{
  printf("%s ", opToString[instruction.op]);
  switch (instruction.op)
  {
  case MOV:
  {
  }
  case ADD:
  {
  }
  case SUB:
  {
  }
  case CMP:
  {
    debugOperand(instruction.operands[0]);
    printf(", ");
    debugOperand(instruction.operands[1]);
    printf("\n");
    break;
  }
  default:
  {
    printf("%d\n", instruction.operands[0].immediate.immediate8);
  }
  }
}

bool read_file(unsigned char** buffer, int* len, const char* fileName)
{
  FILE* filePtr;
  i64   fileSize, count;
  i32   error;

  filePtr = fopen(fileName, "rb");
  if (!filePtr)
  {
    return NULL;
  }

  fileSize            = fseek(filePtr, 0, SEEK_END);
  fileSize            = ftell(filePtr);

  *len                = fileSize;
  *buffer             = (unsigned char*)malloc(sizeof(unsigned char) * (fileSize + 1));
  (*buffer)[fileSize] = '\0';
  fseek(filePtr, 0, SEEK_SET);
  count = fread(*buffer, 1, fileSize, filePtr);
  if (count != fileSize)
  {
    free(*buffer);
    return false;
  }

  error = fclose(filePtr);
  if (error != 0)
  {
    free(*buffer);
    return false;
  }

  return true;
}

void debugByte(ui8 byte)
{
  for (int i = 7; i >= 0; i--)
  {
    printf("%d", byte >> i & 0b1);
  }
  printf("\n");
}

static inline char** getRegisterMemoryEncoding(bool w)
{
  return w ? registerMemoryEncoding1 : registerMemoryEncoding0;
}

static inline char** getRegisterRegisterEncoding(bool w)
{
  return w ? registerToRegisterEncoding16 : registerToRegisterEncoding8;
}

static inline void advance(ui8** buffer)
{
  (*buffer)++;
}

void parseRegMemoryFieldCoding(Operand* operands, char* res, ui8** buffer, ui8 mod, bool w, ui8 rm)
{

  char** encoding = getRegisterMemoryEncoding(mod == 0);
  switch (mod)
  {
  case 0:
  {
    operands->effectiveAddress.mod            = 0;
    operands->effectiveAddress.immediate.size = ZERO;
    operands->effectiveAddress.rm             = rm;
    operands->type                            = EFFECTIVEADDRESS;
    if (rm == 6)
    {
      advance(buffer);
      ui16 direct = *buffer[0];
      if (w)
      {
        advance(buffer);
        direct                                           = (*buffer[0] << 8) | direct;
        operands->effectiveAddress.immediate.size        = SIXTEEN;
        operands->effectiveAddress.immediate.immediate16 = direct;
      }
      else
      {
        operands->effectiveAddress.immediate.size       = EIGHT;
        operands->effectiveAddress.immediate.immediate8 = (ui8)direct;
      }
      sprintf(&res[0], "[%d]", direct);
    }
    else
    {
      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 1:
  {
    operands->effectiveAddress.mod = 1;
    operands->effectiveAddress.rm  = rm;
    operands->type                 = EFFECTIVEADDRESS;

    advance(buffer);
    if (*buffer[0] != 0)
    {
      operands->effectiveAddress.immediate.size       = EIGHT;
      operands->effectiveAddress.immediate.immediate8 = *buffer[0];
      sprintf(&res[0], "[%s + %d]", encoding[rm], *buffer[0]);
    }
    else
    {
      operands->effectiveAddress.immediate.size       = EIGHT;
      operands->effectiveAddress.immediate.immediate8 = 0;

      sprintf(&res[0], "[%s]", encoding[rm]);
    }
    break;
  }
  case 2:
  {
    operands->effectiveAddress.mod = 2;
    operands->effectiveAddress.rm  = rm;
    operands->type                 = EFFECTIVEADDRESS;
    advance(buffer);
    ui16 direct = *buffer[0];

    advance(buffer);
    direct                                           = (*buffer[0] << 8) | direct;

    operands->effectiveAddress.immediate.size        = SIXTEEN;
    operands->effectiveAddress.immediate.immediate16 = direct != 0 ? direct : 0;

    sprintf(&res[0], "[%s + %d]", encoding[rm], direct);
    break;
  }
  case 3:
  {
    operands->type = REGISTER;
    parseRegister(&operands->reg, rm, w);
    sprintf(&res[0], "%s", getRegisterRegisterEncoding(w)[rm]);
    break;
  }
  }
}

static void parseImmediateToRegMemory(Operand* operands, ui8** buffer, ui8 mod, ui8 rm, bool w, bool s, bool mov)
{

  char res[32];
  char source[32];
  parseRegMemoryFieldCoding(&operands[0], &res[0], buffer, mod, w, rm);

  operands[1].type = IMMEDIATE;
  advance(buffer);
  ui16 offset = *buffer[0];
  if (!s && w)
  {
    advance(buffer);
    offset = (*buffer[0] << 8) | offset;
    sprintf(&source[0], mov ? "word %d" : "%d", offset);
    operands[1].immediate.size        = SIXTEEN;
    operands[1].immediate.immediate16 = offset;
  }
  else
  {
    operands[1].immediate.size       = EIGHT;
    operands[1].immediate.immediate8 = offset;
    sprintf(&source[0], mov ? "byte %d" : "%d", offset);
  }
}
static void parseImmediateToReg(Operand* operands, ui8** buffer, ui8 immediate, ui8 reg, bool w)
{
  operands[0].type = REGISTER;
  parseRegister(&operands[0].reg, reg, w);
  operands[1].type = IMMEDIATE;
  if (w)
  {
    advance(buffer);
    ui16 immediate16                  = (*buffer[0] << 8) | immediate;

    operands[1].immediate.size        = SIXTEEN;
    operands[1].immediate.immediate16 = immediate16;
  }
  else
  {
    operands[1].immediate.size       = EIGHT;
    operands[1].immediate.immediate8 = immediate;
  }
}
static void parseRegMemory(Operand* operands, ui8** buffer, char* instruction)
{

  bool d = ((*buffer[0] >> 1) & 1);
  bool w = (*buffer[0] & 1);

  advance(buffer);
  ui8    mod      = (*buffer[0] >> 6) & 0b11;
  ui8    reg      = (*buffer[0] >> 3) & 0b111;
  ui8    rm       = *buffer[0] & 0b111;

  char** encoding = getRegisterRegisterEncoding(w);
  char   res[32];
  parseRegMemoryFieldCoding(&operands[d], &res[0], buffer, mod, w, rm);

  bool nd           = !d;
  operands[nd].type = REGISTER;
  parseRegister(&operands[nd].reg, reg, w);
}

static void parseImmediateToRegMove(Operand* operands, ui8** buffer)
{
  bool w   = (*buffer[0] >> 3 & 1);
  ui8  reg = (*buffer[0] & 0b111);

  advance(buffer);
  ui8 immediate = *buffer[0];

  parseImmediateToReg(operands, buffer, immediate, reg, w);
}

static void parseImmediateToAccumulator(Operand* operands, ui8** buffer, char* instruction)
{

  operands[0].type = ACCUMULATOR;
  operands[1].type = IMMEDIATE;

  bool w           = *buffer[0] & 1;

  advance(buffer);
  ui16 imm = *buffer[0];
  if (w)
  {
    advance(buffer);
    imm                               = (*buffer[0] << 8) | imm;
    operands[1].immediate.size        = SIXTEEN;
    operands[1].immediate.immediate16 = imm;
  }
  else
  {
    operands[1].immediate.size       = EIGHT;
    operands[1].immediate.immediate8 = imm;
  }
}

static void parseImmediateToRegMemoryAddSubCmp(Instruction* instruction, ui8** buffer)
{
  bool s = ((*buffer[0] >> 1) & 1);
  bool w = (*buffer[0] & 1);
  advance(buffer);

  ui8 mod = (*buffer[0] >> 6) & 0b11;
  ui8 reg = (*buffer[0] >> 3) & 0b111;
  ui8 rm  = *buffer[0] & 0b111;

  if (reg == 0b101)
  {
    instruction->op = SUB;
  }
  else if (reg == 0b111)
  {
    instruction->op = CMP;
  }
  else
  {
    instruction->op = ADD;
  }
  parseImmediateToRegMemory(&instruction->operands[0], buffer, mod, rm, w, s, false);
}
static void parseAccumulatorToMemoryMov(Operand* operands, ui8** buffer, bool mta)
{

  bool w = *buffer[0] & 0b1;
  advance(buffer);
  ui16 offset = *buffer[0];
  advance(buffer);
  offset = (*buffer[0] << 8) | offset;
  if (mta)
  {
    operands[0].type                                   = ACCUMULATOR;
    operands[1].type                                   = EFFECTIVEADDRESS;
    operands[1].effectiveAddress.mod                   = 0;
    operands[1].effectiveAddress.rm                    = 6;
    operands[1].effectiveAddress.immediate.size        = SIXTEEN;
    operands[1].effectiveAddress.immediate.immediate16 = offset;
  }
  else
  {
    operands[1].type                                   = ACCUMULATOR;
    operands[0].type                                   = EFFECTIVEADDRESS;
    operands[0].effectiveAddress.mod                   = 0;
    operands[0].effectiveAddress.rm                    = 6;
    operands[0].effectiveAddress.immediate.size        = SIXTEEN;
    operands[0].effectiveAddress.immediate.immediate16 = offset;
  }
}

static void parseImmediateToRegMemoryMove(Operand* operands, ui8** buffer)
{

  bool w = *buffer[0] & 0b1;
  advance(buffer);
  ui8 mod = (*buffer[0] >> 6) & 0b11;
  ui8 rm  = *buffer[0] & 0b111;

  parseImmediateToRegMemory(operands, buffer, mod, rm, w, 0, true);
}

static void parseJump(Operand* operands, ui8** buffer, char* instruction)
{
  advance(buffer);
  operands[0].type                 = IMMEDIATE;
  operands[0].immediate.size       = EIGHT;
  operands[0].immediate.immediate8 = *buffer[0];
}

static inline bool matchImmediateToRegMemoryMove(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b1100011;
}

static inline bool matchImmediateToRegMove(ui8 current)
{
  return ((current >> 4) & 0b1111) == 0b1011;
}

static inline bool matchImmediateToAccumulatorCmp(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b0011110;
}

static inline bool matchImmediateToAccumulatorSub(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b0010110;
}

static inline bool matchImmediateToAccumulatorAdd(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b0000010;
}

static inline bool matchMemoryToAccumulatorMov(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b1010000;
}

static inline bool matchAccumulatorToMemoryMov(ui8 current)
{
  return ((current >> 1) & 0b1111111) == 0b1010001;
}

static inline bool matchRegMemoryCmp(ui8 current)
{
  return ((current >> 2) & 0b111111) == 0b001110;
}

static inline bool matchRegMemorySub(ui8 current)
{
  return ((current >> 2) & 0b111111) == 0b001010;
}

static inline bool matchRegMemoryAdd(ui8 current)
{
  return ((current >> 2) & 0b111111) == 0;
}

static inline bool matchImmediateToRegMemory(ui8 current)
{
  return ((current >> 2) & 0b111111) == 0b100000;
}

static inline bool matchJE(ui8 current)
{
  return current == 0b01110100;
}
static inline bool matchJL(ui8 current)
{
  return current == 0b01111100;
}
static inline bool matchJLE(ui8 current)
{
  return current == 0b01111110;
}
static inline bool matchJB(ui8 current)
{
  return current == 0b01110010;
}
static inline bool matchJBE(ui8 current)
{
  return current == 0b01110110;
}
static inline bool matchJP(ui8 current)
{
  return current == 0b01111010;
}
static inline bool matchJO(ui8 current)
{
  return current == 0b01110000;
}
static inline bool matchJS(ui8 current)
{
  return current == 0b01111000;
}
static inline bool matchJNE(ui8 current)
{
  return current == 0b01110101;
}
static inline bool matchJNL(ui8 current)
{
  return current == 0b01111101;
}
static inline bool matchJNLE(ui8 current)
{
  return current == 0b01111111;
}
static inline bool matchJNB(ui8 current)
{
  return current == 0b01110011;
}
static inline bool matchJNBE(ui8 current)
{
  return current == 0b01110111;
}
static inline bool matchJNP(ui8 current)
{
  return current == 0b01111011;
}
static inline bool matchJNO(ui8 current)
{
  return current == 0b01110001;
}
static inline bool matchJNS(ui8 current)
{
  return current == 0b01111001;
}
static inline bool matchLoop(ui8 current)
{
  return current == 0b11100010;
}
static inline bool matchLoopz(ui8 current)
{
  return current == 0b11100001;
}
static inline bool matchLoopnz(ui8 current)
{
  return current == 0b11100000;
}
static inline bool matchJCXZ(ui8 current)
{
  return current == 0b11100011;
}
static inline bool matchRegMemoryMove(ui8 current)
{
  return (current >> 2 & 0b111111) == 0b100010;
}

void setRegisterValue(ui16* reg, ui16 value, ImmediateSize size, ui8 offset)
{
  if (offset == 0)
  {
    if (size == EIGHT)
    {
      ui8 v = (ui8)value;
      *reg  = *reg & 0x00FF;
      *reg  = *reg | v;
    }
    else
    {
      *reg = value;
    }
  }
  else
  {
    ui8 v = (ui8)value;
    *reg  = *reg & 0xFF00;
    *reg  = *reg | (v << 8);
  }
}

ui16 getOperandValue(ui16* registers, Operand operand)
{
  if (operand.type == IMMEDIATE)
  {
    return operand.immediate.size == EIGHT ? operand.immediate.immediate16 : operand.immediate.immediate8;
  }
  else if (operand.type == REGISTER)
  {
    ui16 regValue = registers[operand.reg.type];
    if (operand.reg.size == SIXTEEN)
    {
      return regValue;
    }
    if (operand.reg.offset == 8)
    {
      return regValue << 8;
    }
    return regValue & 0xFF;
  }

  return 0;
}

void executeMoveInstruction(ui16* registers, Operand* operands)
{
  if (operands[0].type == REGISTER)
  {
    ui16 value     = getOperandValue(registers, operands[1]);
    ui16 prevValue = registers[operands[0].reg.type];
    setRegisterValue(&registers[operands[0].reg.type], value, operands[1].immediate.size, operands[0].reg.offset);
    printf("\t%s\t0x%04x -> 0x%04x\n", registerNames[operands[0].reg.type], prevValue, registers[operands[0].reg.type]);
  }
}

void executeInstruction(ui16* registers, Instruction instruction)
{
  switch (instruction.op)
  {
  case MOV:
  {
    executeMoveInstruction(registers, instruction.operands);
  }
  }
}

int main()
{
  ui8*        buffer;
  ui8*        end;
  int         len;
  const char* name = "t7";
  read_file(&buffer, &len, name);

  end = buffer + len;
  printf("; %s.asm\n", name);
  printf("bits 16\n\n");

  Instruction instructions[256];
  ui16        current = 0;

  ui16        registers[8];

  while (buffer < end)
  {
    if (matchRegMemoryAdd(buffer[0]))
    {
      instructions[current].op = ADD;
      parseRegMemory(&instructions[current].operands[0], &buffer, "add");
    }
    else if (matchRegMemoryCmp(buffer[0]))
    {
      instructions[current].op = CMP;
      parseRegMemory(&instructions[current].operands[0], &buffer, "cmp");
    }
    else if (matchRegMemorySub(buffer[0]))
    {
      instructions[current].op = SUB;
      parseRegMemory(&instructions[current].operands[0], &buffer, "sub");
    }
    else if (matchImmediateToRegMemory(buffer[0]))
    {
      parseImmediateToRegMemoryAddSubCmp(&instructions[current], &buffer);
    }
    else if (matchImmediateToAccumulatorCmp(buffer[0]))
    {
      instructions[current].op = CMP;
      parseImmediateToAccumulator(&instructions[current].operands[0], &buffer, "cmp");
    }
    else if (matchImmediateToAccumulatorSub(buffer[0]))
    {
      instructions[current].op = SUB;
      parseImmediateToAccumulator(&instructions[current].operands[0], &buffer, "sub");
    }
    else if (matchImmediateToAccumulatorAdd(buffer[0]))
    {
      instructions[current].op = ADD;
      parseImmediateToAccumulator(&instructions[current].operands[0], &buffer, "add");
    }
    else if (matchImmediateToRegMove(buffer[0]))
    {
      instructions[current].op = MOV;
      parseImmediateToRegMove(&instructions[current].operands[0], &buffer);
    }
    else if (matchAccumulatorToMemoryMov(buffer[0]))
    {
      instructions[current].op = MOV;
      parseAccumulatorToMemoryMov(&instructions[current].operands[0], &buffer, false);
    }
    else if (matchMemoryToAccumulatorMov(buffer[0]))
    {
      instructions[current].op = MOV;
      parseAccumulatorToMemoryMov(&instructions[current].operands[0], &buffer, true);
    }
    else if (matchImmediateToRegMemoryMove(buffer[0]))
    {
      instructions[current].op = MOV;
      parseImmediateToRegMemoryMove(&instructions[current].operands[0], &buffer);
    }
    else if (matchRegMemoryMove(buffer[0]))
    {
      instructions[current].op = MOV;
      parseRegMemory(&instructions[current].operands[0], &buffer, "mov");
    }
    else if (matchJE(buffer[0]))
    {
      instructions[current].op = JE;
      parseJump(&instructions[current].operands[0], &buffer, "je");
    }
    else if (matchJL(buffer[0]))
    {
      instructions[current].op = JL;
      parseJump(&instructions[current].operands[0], &buffer, "jl");
    }
    else if (matchJLE(buffer[0]))
    {
      instructions[current].op = JLE;
      parseJump(&instructions[current].operands[0], &buffer, "jle");
    }
    else if (matchJB(buffer[0]))
    {
      instructions[current].op = JB;
      parseJump(&instructions[current].operands[0], &buffer, "jb");
    }
    else if (matchJBE(buffer[0]))
    {
      instructions[current].op = JBE;
      parseJump(&instructions[current].operands[0], &buffer, "jbe");
    }
    else if (matchJP(buffer[0]))
    {
      instructions[current].op = JP;
      parseJump(&instructions[current].operands[0], &buffer, "jp");
    }
    else if (matchJO(buffer[0]))
    {
      instructions[current].op = JO;
      parseJump(&instructions[current].operands[0], &buffer, "jo");
    }
    else if (matchJS(buffer[0]))
    {
      instructions[current].op = JS;
      parseJump(&instructions[current].operands[0], &buffer, "js");
    }
    else if (matchJNE(buffer[0]))
    {
      instructions[current].op = JNE;
      parseJump(&instructions[current].operands[0], &buffer, "jne");
    }
    else if (matchJNL(buffer[0]))
    {
      instructions[current].op = JNL;
      parseJump(&instructions[current].operands[0], &buffer, "jnl");
    }
    else if (matchJNLE(buffer[0]))
    {
      instructions[current].op = JNLE;
      parseJump(&instructions[current].operands[0], &buffer, "jnle");
    }
    else if (matchJNB(buffer[0]))
    {
      instructions[current].op = JNB;
      parseJump(&instructions[current].operands[0], &buffer, "jnb");
    }
    else if (matchJNBE(buffer[0]))
    {
      instructions[current].op = JNBE;
      parseJump(&instructions[current].operands[0], &buffer, "jnbe");
    }
    else if (matchJNP(buffer[0]))
    {
      instructions[current].op = JP;
      parseJump(&instructions[current].operands[0], &buffer, "jp");
    }
    else if (matchJNO(buffer[0]))
    {
      instructions[current].op = JNO;
      parseJump(&instructions[current].operands[0], &buffer, "jno");
    }
    else if (matchJNS(buffer[0]))
    {
      instructions[current].op = JNS;
      parseJump(&instructions[current].operands[0], &buffer, "jns");
    }
    else if (matchLoop(buffer[0]))
    {
      instructions[current].op = LOOP;
      parseJump(&instructions[current].operands[0], &buffer, "loop");
    }
    else if (matchLoopz(buffer[0]))
    {
      instructions[current].op = LOOPZ;
      parseJump(&instructions[current].operands[0], &buffer, "loopz");
    }
    else if (matchLoopnz(buffer[0]))
    {
      instructions[current].op = LOOPNZ;
      parseJump(&instructions[current].operands[0], &buffer, "loopnz");
    }
    else if (matchJCXZ(buffer[0]))
    {
      instructions[current].op = JCXZ;
      parseJump(&instructions[current].operands[0], &buffer, "jcxz");
    }
    else
    {
      printf("UNKNOWN INSTRUCTION ");
      debugByte(buffer[0]);
      exit(1);
    }
    // debugInstruction(instructions[current]);
    executeInstruction(registers, instructions[current]);
    current++;
    buffer++;
  }
}
