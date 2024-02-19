#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <x86intrin.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float    f32;
typedef double   f64;

struct Profiler
{
  const char* name[100];
  u64         timeStart[100];
  u64         timeEnd[100];
  bool        done[100];
  u64         count;
};
typedef struct Profiler Profiler;
extern Profiler         profiler;
void                    initProfiler();
void                    atExit(int* really);
void                    displayProfilingResult();

#define TimeBlock(blockName)                                                                                                                                                                           \
  profiler.name[profiler.count]      = (char*)malloc(sizeof(char) * strlen(blockName));                                                                                                                \
  profiler.name[profiler.count]      = blockName;                                                                                                                                                      \
  profiler.timeStart[profiler.count] = ReadCPUTimer();                                                                                                                                                 \
  profiler.done[profiler.count]      = false;                                                                                                                                                          \
  profiler.count++;                                                                                                                                                                                    \
  int EXIT_VAR __attribute__((__cleanup__(atExit))) = 1;

#define TimeFunction                                                                                                                                                                                   \
  profiler.name[profiler.count]      = (char*)malloc(sizeof(char) * strlen(__FUNCTION__));                                                                                                             \
  profiler.name[profiler.count]      = __FUNCTION__;                                                                                                                                                   \
  profiler.timeStart[profiler.count] = ReadCPUTimer();                                                                                                                                                 \
  profiler.done[profiler.count]      = false;                                                                                                                                                          \
  profiler.count++;                                                                                                                                                                                    \
  int EXIT_VAR __attribute__((__cleanup__(atExit))) = 1;
u64 ReadCPUTimer(void);
u64 EstimateCPUTimerFreq(void);

#endif
