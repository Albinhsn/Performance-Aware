#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef bool     u1;

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

#define CF                         0
#define PF                         1
#define AF                         2
#define ZF                         3
#define SF                         4
#define GETZF(flags)               ((flags >> (ZF - 1)) & 0b1)
#define GETSF(flags)               ((flags >> (SF - 1)) & 0b1)

#define SETZF(flags)               (flags = flags | 0b100)
#define SETSF(flags)               (flags = flags | 0b1000)

#define GET_AX(cpu)                (cpu->registers[0])
#define GET_CX(cpu)                (cpu->registers[1])
#define GET_DX(cpu)                (cpu->registers[2])
#define GET_BX(cpu)                (cpu->registers[3])
#define GET_SX(cpu)                (cpu->registers[4])
#define GET_BP(cpu)                (cpu->registers[5])
#define GET_SI(cpu)                (cpu->registers[6])
#define GET_DI(cpu)                (cpu->registers[7])

#define IMMEDIATE_VALUE(immediate) (immediate.size == EIGHT ? immediate.immediate8 : immediate.immediate16)

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
  JNZ,
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

#define NUMBER_OF_REGISTERS 8
char* registerNames[NUMBER_OF_REGISTERS] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

void  debugFlags(ui8 flags)
{
  bool f = false;
  if (GETZF(flags))
  {
    printf(" Z");
    f = true;
  }
  if (GETSF(flags))
  {
    printf(" S");
    f = true;
  }
}
void debugRegisters(ui16* registers)
{
  for (i32 i = 0; i < NUMBER_OF_REGISTERS; i++)
  {
    if (registers[i] != 0)
    {
      printf("\t%s: x0%04x(%d)\n", registerNames[i], registers[i], registers[i]);
    }
  }
}

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
  RegisterType  type;
  ui8           offset;
  ImmediateSize size;
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

struct CPU
{
  ui16*       registers;
  ui8         flags;
  ui8         prevFlags;
  Instruction instruction;
  ui8*        start;
  ui8*        prev;
  ui16        cycles;
  ui8         memory[1024 * 1024];
};
typedef struct CPU CPU;

RegisterType       registerEncoded[8] = {A, C, D, B, SP, BP, SI, DI};

void               parseRegister(Register* regis, ui8 reg, bool w)
{
  if (w)
  {
    regis->offset = 0;
    regis->size   = SIXTEEN;
    regis->type   = registerEncoded[reg];
  }
  else if (reg <= 3)
  {
    regis->offset = 0;
    regis->size   = EIGHT;
    regis->type   = registerEncoded[reg];
  }
  else
  {
    regis->type   = registerEncoded[reg - 4];
    regis->size   = EIGHT;
    regis->offset = 8;
  }
}
const char* opToString[]  = {"mov", "add", "sub", "cmp", "je", "jl", "jle", "jb", "jbe", "jp", "jo", "js", "jnz", "jnl", "jnle", "jnb", "jnbe", "jnp", "jno", "jns", "loop", "loopz", "loopnz", "jcxz"};

const char* regToString[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char* regToStringHigh[] = {"ah", "ch", "dh", "bh"};
const char* regToStringLow[]  = {"al", "cl", "dl", "bl"};

void        debugRegister(Register reg)
{
  if (reg.size == SIXTEEN)
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
    printf("%d", effectiveAddress.immediate.size == EIGHT ? effectiveAddress.immediate.immediate8 : effectiveAddress.immediate.immediate16);
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

void debugIp(CPU* cpu, ui8* buffer)
{
  printf("\t\tip:0x%04x->0x%04x(%d)", (int)(cpu->prev - cpu->start), (i32)(buffer - cpu->start), (i32)(buffer - cpu->start));
}
static inline ui16 calculateEffectiveAddress(CPU* cpu, ui8 rm)
{
  switch (rm)
  {
  case 0:
  {
    return GET_BX(cpu) + GET_SI(cpu);
  }
  case 1:
  {
    return GET_BX(cpu) + GET_DI(cpu);
  }
  case 2:
  {
    return GET_BP(cpu) + GET_SI(cpu);
  }
  case 3:
  {
    return GET_BP(cpu) + GET_DI(cpu);
  }
  case 4:
  {
    return GET_SI(cpu);
  }
  case 5:
  {
    return GET_DI(cpu);
  }
  case 6:
  {
    return GET_BP(cpu);
  }
  case 7:
  {
    return GET_BX(cpu);
  }
  default:
  {
    printf("HUH\n");
    exit(1);
  }
  }
}
static ui16 getEffectiveAddress(CPU* cpu, EffectiveAddress effectiveAddress)
{
  if (effectiveAddress.mod == 0 && effectiveAddress.rm == 6)
  {
    return IMMEDIATE_VALUE(effectiveAddress.immediate);
  }
  ui16 effective = calculateEffectiveAddress(cpu, effectiveAddress.rm);
  if (effectiveAddress.mod == 0)
  {
    return effective;
  }
  if (effectiveAddress.mod == 1)
  {
    return effective + effectiveAddress.immediate.immediate8;
  }

  return effective + effectiveAddress.immediate.immediate16;
}

struct Cycles
{
  ui8 normal;
  ui8 ea;
  ui8 penalty;
};
typedef struct Cycles Cycles;

ui8                   effectiveAddressNoDisplacementTable[8] = {
    7, // BX + SI
    8, // BX + DI
    8, // BP + SI
    7, // BP + DI
    5, // SI
    5, // DI
    6, // DIRECT ADDRESS
    5, // BX
};
ui8 effectiveAddressDisplacementTable[8] = {

    11, // BX + SI + DISP
    12, // BX + DI + DISP
    12, // BP + SI + DISP
    11, // BP + DI + DISP
    9,  // SI + DISP
    9,  // DI + DISP
    9,  // BP + DISP
    9,  // BX + DISP
};

void calcPenalty(CPU* cpu, Cycles* cycles, ui8 transfers, EffectiveAddress effectiveAddress)
{
  if (effectiveAddress.immediate.size == SIXTEEN && getEffectiveAddress(cpu, effectiveAddress) % 2 != 0)
  {
    cycles->penalty += transfers * 4;
  }
}

ui8 calcEffectiveAddressCycles(CPU* cpu, EffectiveAddress effectiveAddress)
{
  if (effectiveAddress.mod == 1)
  {
    return effectiveAddressNoDisplacementTable[effectiveAddress.rm];
  }
  else if (effectiveAddress.immediate.size == ZERO)
  {
    if (effectiveAddress.rm == 6)
    {
      return 5;
    }
    return effectiveAddressNoDisplacementTable[effectiveAddress.rm];
  }
  return effectiveAddressDisplacementTable[effectiveAddress.rm];
}

Cycles calcCyclesAdd(CPU* cpu, Operand* operands)
{
  Cycles cycles;
  cycles.normal  = 0;
  cycles.ea      = 0;
  cycles.penalty = 0;
  switch (operands[0].type)
  {
  case REGISTER:
  {
    if (operands[1].type == REGISTER)
    {
      cycles.normal = 3;
    }
    else if (operands[1].type == EFFECTIVEADDRESS)
    {
      cycles.normal = 9;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[1].effectiveAddress);
      calcPenalty(cpu, &cycles, 1, operands[1].effectiveAddress);
    }
    else if (operands[1].type == IMMEDIATE)
    {
      cycles.normal = 4;
    }
    else
    {
      printf("HUH\n");
      exit(1);
    }
    break;
  }
  case IMMEDIATE:
  {
    printf("HUH\n");
    exit(1);
  }
  case EFFECTIVEADDRESS:
  {
    if (operands[1].type == REGISTER)
    {
      cycles.normal = 16;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[0].effectiveAddress);
      calcPenalty(cpu, &cycles, 2, operands[0].effectiveAddress);
    }
    else if (operands[1].type == IMMEDIATE)
    {
      cycles.normal = 17;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[0].effectiveAddress);
      calcPenalty(cpu, &cycles, 2, operands[0].effectiveAddress);
    }
    else
    {
      printf("HUH\n");
      exit(1);
    }
    break;
  }
  case ACCUMULATOR:
  {
    cycles.normal = 4;
  }
  }
  return cycles;
}

Cycles calcCyclesMov(CPU* cpu, Operand* operands)
{
  Cycles cycles;
  cycles.normal  = 0;
  cycles.ea      = 0;
  cycles.penalty = 0;
  switch (operands[0].type)
  {
  case REGISTER:
  {
    if (operands[1].type == REGISTER)
    {
      cycles.normal = 2;
    }
    else if (operands[1].type == EFFECTIVEADDRESS)
    {
      cycles.normal = 8;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[1].effectiveAddress);
      calcPenalty(cpu, &cycles, 1, operands[1].effectiveAddress);
    }
    else if (operands[1].type == IMMEDIATE)
    {
      cycles.normal = 4;
    }
    else
    {
      printf("HUH\n");
      exit(1);
    }
    break;
  }
  case IMMEDIATE:
  {
    printf("HUH\n");
    exit(1);
  }
  case EFFECTIVEADDRESS:
  {
    if (operands[1].type == REGISTER)
    {
      cycles.normal = 9;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[0].effectiveAddress);
      calcPenalty(cpu, &cycles, 1, operands[0].effectiveAddress);
    }
    else if (operands[1].type == IMMEDIATE)
    {
      cycles.normal = 10;
      cycles.ea     = calcEffectiveAddressCycles(cpu, operands[0].effectiveAddress);
      calcPenalty(cpu, &cycles, 1, operands[0].effectiveAddress);
    }
    else if (operands[1].type == ACCUMULATOR)
    {
      cycles.normal = 10;
    }
    else
    {
      printf("HUH\n");
      exit(1);
    }
    break;
  }
  case ACCUMULATOR:
  {
    cycles.normal = 10;
  }
  }
  return cycles;
}

Cycles calcCycles(CPU* cpu)
{
  switch (cpu->instruction.op)
  {
  case MOV:
  {
    return calcCyclesMov(cpu, cpu->instruction.operands);
  }
  case ADD:
  {
    return calcCyclesAdd(cpu, cpu->instruction.operands);
  }
  case SUB:
  {
  }
  case CMP:
  {
  }
  default:
  {
  }
  }
  return (Cycles){.normal = 0, .ea = 0};
}

void debugInstruction(CPU* cpu, ui8* buffer)
{

  printf("%s ", opToString[cpu->instruction.op]);
  switch (cpu->instruction.op)
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
    debugOperand(cpu->instruction.operands[0]);
    printf(", ");
    debugOperand(cpu->instruction.operands[1]);
    printf("; ");
    break;
  }

  default:
  {
    printf("%d;", *(i8*)&cpu->instruction.operands[0].immediate.immediate8);
    break;
  }
  }
  // debugIp(cpu, buffer);
  // if (cpu->flags != cpu->prevFlags)
  // {
  //   printf("\t\tflags:");
  //   debugFlags(cpu->prevFlags);
  //   printf("->");
  //   debugFlags(cpu->flags);
  // }
  // printf("\n");
  Cycles cycles = calcCycles(cpu);
  printf("\tcycles: %d", cycles.normal + cycles.ea);
  if (cycles.ea || cycles.penalty)
  {
    printf("(%d", cycles.normal);
    if (cycles.ea)
    {
      printf(", ea: %d", cycles.ea);
    }
    if (cycles.penalty)
    {
      printf(", p: %d", cycles.penalty);
    }
    printf(")");
  }
  cpu->cycles += cycles.normal + cycles.ea + cycles.penalty;
  printf(" -> total: %d\n", cpu->cycles);
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

void updateFlags(CPU* cpu, ui16 value)
{
  ui16 prev  = cpu->flags;
  cpu->flags = 0;
  if (value == 0)
  {
    SETZF(cpu->flags);
  }
  else if ((value & 0x8000) != 0)
  {
    SETSF(cpu->flags);
  }
}

void setRegisterValue(CPU* cpu, Register* reg, ui16 value)
{
  ui16* regValue  = &cpu->registers[reg->type];
  ui16  prevValue = *regValue;
  if (reg->offset == 0)
  {
    if (reg->size == EIGHT)
    {
      ui8 v     = (ui8)value;
      *regValue = *regValue & 0x00FF;
      *regValue = *regValue | v;
    }
    else
    {
      *regValue = value;
    }
  }
  else
  {
    ui8 v     = (ui8)value;
    *regValue = *regValue & 0xFF00;
    *regValue = *regValue | (v << 8);
  }
}

Immediate getOperandValue(CPU* cpu, Operand operand)
{
  Immediate immediate;
  if (operand.type == IMMEDIATE)
  {
    return operand.immediate;
  }
  else if (operand.type == REGISTER)
  {
    ui16 regValue = cpu->registers[operand.reg.type];
    if (operand.reg.size == SIXTEEN)
    {
      immediate.size        = SIXTEEN;
      immediate.immediate16 = regValue;
    }
    else
    {
      immediate.size       = EIGHT;
      immediate.immediate8 = operand.reg.offset == 8 ? regValue << 8 : regValue & 0xFF;
    }
  }
  else if (operand.type == EFFECTIVEADDRESS)
  {
    ui16 effectiveAddress = getEffectiveAddress(cpu, operand.effectiveAddress);
    immediate.size        = operand.effectiveAddress.immediate.size;
    if (immediate.size == EIGHT)
    {
      immediate.immediate8 = cpu->memory[effectiveAddress];
    }
    else
    {
      immediate.immediate16 = *(ui16*)&cpu->memory[effectiveAddress];
    }
  }
  return immediate;
}

static inline void setMemoryValue(CPU* cpu, ui16 dest, Immediate value)
{
  if (value.size == SIXTEEN)
  {
    cpu->memory[dest]     = value.immediate16 & 0xFF;
    cpu->memory[dest + 1] = value.immediate16 << 8;
  }
  else
  {
    cpu->memory[dest] = value.immediate8;
  }
}

void executeMoveInstruction(CPU* cpu, Operand* operands)
{
  Immediate immediate = getOperandValue(cpu, operands[1]);
  if (operands[0].type == REGISTER)
  {
    setRegisterValue(cpu, &operands[0].reg, immediate.size == EIGHT ? immediate.immediate8 : immediate.immediate16);
  }
  else if (operands[0].type == EFFECTIVEADDRESS)
  {
    ui16 dest = getEffectiveAddress(cpu, operands[0].effectiveAddress);
    setMemoryValue(cpu, dest, immediate);
  }
}

void executeAddInstruction(CPU* cpu, Operand* operands)
{
  Immediate op1 = getOperandValue(cpu, operands[1]);
  Immediate op0 = getOperandValue(cpu, operands[0]);
  ui16      res = IMMEDIATE_VALUE(op0) + IMMEDIATE_VALUE(op1);
  if (operands[0].type == REGISTER)
  {
    setRegisterValue(cpu, &operands[0].reg, res);
    updateFlags(cpu, res);
  }
}
void executeJumpNotZeroInstruction(CPU* cpu, Operand* operands, ui8** buffer)
{
  if (!GETZF(cpu->flags))
  {
    *buffer += *(i8*)&operands[0].immediate.immediate8;
  }
}

void executeCmpInstruction(CPU* cpu, Operand* operands)
{
  Immediate op1 = getOperandValue(cpu, operands[1]);
  Immediate op0 = getOperandValue(cpu, operands[0]);
  ui16      res = IMMEDIATE_VALUE(op0) - IMMEDIATE_VALUE(op1);

  if (operands[0].type == REGISTER)
  {
    updateFlags(cpu, res);
  }
}

void executeSubInstruction(CPU* cpu, Operand* operands)
{
  Immediate op1 = getOperandValue(cpu, operands[1]);
  Immediate op0 = getOperandValue(cpu, operands[0]);
  ui16      res = IMMEDIATE_VALUE(op0) - IMMEDIATE_VALUE(op1);
  if (operands[0].type == REGISTER)
  {
    setRegisterValue(cpu, &operands[0].reg, res);
    updateFlags(cpu, res);
  }
}

void executeInstruction(CPU* cpu, Instruction instruction, ui8** buffer)
{
  switch (instruction.op)
  {
  case MOV:
  {
    executeMoveInstruction(cpu, instruction.operands);
    break;
  }
  case ADD:
  {
    executeAddInstruction(cpu, instruction.operands);
    break;
  }
  case SUB:
  {
    executeSubInstruction(cpu, instruction.operands);
    break;
  }
  case CMP:
  {
    executeCmpInstruction(cpu, instruction.operands);
    break;
  }
  case JNZ:
  {
    executeJumpNotZeroInstruction(cpu, instruction.operands, buffer);
    break;
  }
  default:
  {
    printf("\n");
    break;
  }
  }
}

#define MEMORY_SIZE 1024 * 1024
void debugMemory(CPU* cpu)
{
  printf("memory:\n");
  for (i32 i = 0; i < MEMORY_SIZE; i++)
  {
    if (cpu->memory[i] != 0)
    {
      printf("%d", cpu->memory[i]);
    }
  }
}

int main()
{
  ui8*        buffer;
  ui8*        end;
  int         len;
  const char* name = "listing_57";
  read_file(&buffer, &len, name);

  end = buffer + len;
  // printf("; %s.asm\n", name);
  // printf("bits 16\n\n");

  ui16 registers[NUMBER_OF_REGISTERS];
  for (i32 i = 0; i < NUMBER_OF_REGISTERS; i++)
  {
    registers[i] = 0;
  }
  CPU cpu;
  cpu.registers = &registers[0];
  cpu.prevFlags = 0;
  cpu.flags     = 0;
  cpu.start     = buffer;
  cpu.prev      = buffer;
  cpu.cycles    = 0;
  for (i32 i = 0; i < MEMORY_SIZE; i++)
  {
    cpu.memory[i] = 0;
  }

  while (buffer < end)
  {
    if (matchRegMemoryAdd(buffer[0]))
    {
      cpu.instruction.op = ADD;
      parseRegMemory(&cpu.instruction.operands[0], &buffer, "add");
    }
    else if (matchRegMemoryCmp(buffer[0]))
    {
      cpu.instruction.op = CMP;
      parseRegMemory(&cpu.instruction.operands[0], &buffer, "cmp");
    }
    else if (matchRegMemorySub(buffer[0]))
    {
      cpu.instruction.op = SUB;
      parseRegMemory(&cpu.instruction.operands[0], &buffer, "sub");
    }
    else if (matchImmediateToRegMemory(buffer[0]))
    {
      parseImmediateToRegMemoryAddSubCmp(&cpu.instruction, &buffer);
    }
    else if (matchImmediateToAccumulatorCmp(buffer[0]))
    {
      cpu.instruction.op = CMP;
      parseImmediateToAccumulator(&cpu.instruction.operands[0], &buffer, "cmp");
    }
    else if (matchImmediateToAccumulatorSub(buffer[0]))
    {
      cpu.instruction.op = SUB;
      parseImmediateToAccumulator(&cpu.instruction.operands[0], &buffer, "sub");
    }
    else if (matchImmediateToAccumulatorAdd(buffer[0]))
    {
      cpu.instruction.op = ADD;
      parseImmediateToAccumulator(&cpu.instruction.operands[0], &buffer, "add");
    }
    else if (matchImmediateToRegMove(buffer[0]))
    {
      cpu.instruction.op = MOV;
      parseImmediateToRegMove(&cpu.instruction.operands[0], &buffer);
    }
    else if (matchAccumulatorToMemoryMov(buffer[0]))
    {
      cpu.instruction.op = MOV;
      parseAccumulatorToMemoryMov(&cpu.instruction.operands[0], &buffer, false);
    }
    else if (matchMemoryToAccumulatorMov(buffer[0]))
    {
      cpu.instruction.op = MOV;
      parseAccumulatorToMemoryMov(&cpu.instruction.operands[0], &buffer, true);
    }
    else if (matchImmediateToRegMemoryMove(buffer[0]))
    {
      cpu.instruction.op = MOV;
      parseImmediateToRegMemoryMove(&cpu.instruction.operands[0], &buffer);
    }
    else if (matchRegMemoryMove(buffer[0]))
    {
      cpu.instruction.op = MOV;
      parseRegMemory(&cpu.instruction.operands[0], &buffer, "mov");
    }
    else if (matchJE(buffer[0]))
    {
      cpu.instruction.op = JE;
      parseJump(&cpu.instruction.operands[0], &buffer, "je");
    }
    else if (matchJL(buffer[0]))
    {
      cpu.instruction.op = JL;
      parseJump(&cpu.instruction.operands[0], &buffer, "jl");
    }
    else if (matchJLE(buffer[0]))
    {
      cpu.instruction.op = JLE;
      parseJump(&cpu.instruction.operands[0], &buffer, "jle");
    }
    else if (matchJB(buffer[0]))
    {
      cpu.instruction.op = JB;
      parseJump(&cpu.instruction.operands[0], &buffer, "jb");
    }
    else if (matchJBE(buffer[0]))
    {
      cpu.instruction.op = JBE;
      parseJump(&cpu.instruction.operands[0], &buffer, "jbe");
    }
    else if (matchJP(buffer[0]))
    {
      cpu.instruction.op = JP;
      parseJump(&cpu.instruction.operands[0], &buffer, "jp");
    }
    else if (matchJO(buffer[0]))
    {
      cpu.instruction.op = JO;
      parseJump(&cpu.instruction.operands[0], &buffer, "jo");
    }
    else if (matchJS(buffer[0]))
    {
      cpu.instruction.op = JS;
      parseJump(&cpu.instruction.operands[0], &buffer, "js");
    }
    else if (matchJNE(buffer[0]))
    {
      cpu.instruction.op = JNZ;
      parseJump(&cpu.instruction.operands[0], &buffer, "jne");
    }
    else if (matchJNL(buffer[0]))
    {
      cpu.instruction.op = JNL;
      parseJump(&cpu.instruction.operands[0], &buffer, "jnl");
    }
    else if (matchJNLE(buffer[0]))
    {
      cpu.instruction.op = JNLE;
      parseJump(&cpu.instruction.operands[0], &buffer, "jnle");
    }
    else if (matchJNB(buffer[0]))
    {
      cpu.instruction.op = JNB;
      parseJump(&cpu.instruction.operands[0], &buffer, "jnb");
    }
    else if (matchJNBE(buffer[0]))
    {
      cpu.instruction.op = JNBE;
      parseJump(&cpu.instruction.operands[0], &buffer, "jnbe");
    }
    else if (matchJNP(buffer[0]))
    {
      cpu.instruction.op = JP;
      parseJump(&cpu.instruction.operands[0], &buffer, "jp");
    }
    else if (matchJNO(buffer[0]))
    {
      cpu.instruction.op = JNO;
      parseJump(&cpu.instruction.operands[0], &buffer, "jno");
    }
    else if (matchJNS(buffer[0]))
    {
      cpu.instruction.op = JNS;
      parseJump(&cpu.instruction.operands[0], &buffer, "jns");
    }
    else if (matchLoop(buffer[0]))
    {
      cpu.instruction.op = LOOP;
      parseJump(&cpu.instruction.operands[0], &buffer, "loop");
    }
    else if (matchLoopz(buffer[0]))
    {
      cpu.instruction.op = LOOPZ;
      parseJump(&cpu.instruction.operands[0], &buffer, "loopz");
    }
    else if (matchLoopnz(buffer[0]))
    {
      cpu.instruction.op = LOOPNZ;
      parseJump(&cpu.instruction.operands[0], &buffer, "loopnz");
    }
    else if (matchJCXZ(buffer[0]))
    {
      cpu.instruction.op = JCXZ;
      parseJump(&cpu.instruction.operands[0], &buffer, "jcxz");
    }
    else
    {
      printf("UNKNOWN INSTRUCTION ");
      debugByte(buffer[0]);
      exit(1);
    }
    buffer++;
    executeInstruction(&cpu, cpu.instruction, &buffer);
    debugInstruction(&cpu, buffer);

    cpu.prevFlags = cpu.flags;
    cpu.prev      = buffer;
  }
  FILE* filePtr;
  filePtr = fopen("test.data", "w");
  fwrite(cpu.memory, MEMORY_SIZE, 1, filePtr);
  fclose(filePtr);
  // printf("Final stuff:\n");
  // debugRegisters(registers);
  // debugIp(&cpu, buffer);
  // printf("\n\tflags: ");
  // debugFlags(cpu.flags);
  // printf("\n");
}
