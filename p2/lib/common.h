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

struct Profile
{
  const char* name;
  u64         timeStart;
  u64         timeEnd;
};
typedef struct Profile Profile;
struct Profiler
{
  Profile profiles[100];
  u64     count;
  u64     timeStart;
  u64     timeEnd;
};
typedef struct Profiler Profiler;
extern Profiler         profiler;
void                    initProfiler();
void                    atExit(int* really);
void                    displayProfilingResult();

#define TimeBlock(blockName)                                                                                                                                                                           \
  profiler.count++;                                                                                                                                                                                    \
  profiler.profiles[profiler.count].name      = blockName;                                                                                                                                             \
  profiler.profiles[profiler.count].timeStart = ReadCPUTimer();                                                                                                                                        \
  int EXIT_VAR __attribute__((__cleanup__(atExit))) = profiler.count;

#define TimeFunction                                                                                                                                                                                   \
  profiler.count++;                                                                                                                                                                                    \
  profiler.profiles[profiler.count].name      = __FUNCTION__;                                                                                                                                          \
  profiler.profiles[profiler.count].timeStart = ReadCPUTimer();                                                                                                                                        \
  int EXIT_VAR __attribute__((__cleanup__(atExit))) = profiler.count;

u64 ReadCPUTimer(void);
u64 EstimateCPUTimerFreq(void);

#endif
