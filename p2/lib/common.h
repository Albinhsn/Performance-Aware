#ifndef COMMON_H
#define COMMON_H

#include <cstdio>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <x86intrin.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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

u64              ReadCPUTimer(void);
u64              EstimateCPUTimerFreq(void);

struct Profile
{
  u64         timeElapsed;
  u64         hitCount;
  const char* name;
};

struct Profiler
{
  Profile profiles[100];
  u32     count;
  u64     timeStart;
};
extern Profiler profiler;

void            initProfiler();
void            atExit(int* really);
void            displayProfilingResult();

struct ProfileBlock
{
  const char* name;
  u64         timeStart;
  u32         index;

  ProfileBlock(const char* name_)
  {
    name      = name_;
    timeStart = ReadCPUTimer();
    index     = profiler.count;
    profiler.count++;
  }

  ~ProfileBlock()
  {
    u64      elapsed = ReadCPUTimer() - timeStart;
    Profile* profile = profiler.profiles + index;
    profile->name    = name;
    profile->hitCount++;
    profile->timeElapsed += elapsed;
  }
};

#define NameConcat2(A, B)    A##B
#define NameConcat(A, B)     NameConcat2(A, B)
#define TimeBlock(blockName) ProfileBlock NameConcat(Block, __LINE__)(blockName);
#define TimeFunction         TimeBlock(__func__)

#endif
