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
  u64         timeElapsedChildren;
  u64         timeElapsedRoot;
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
extern u32      profilerParent;

void            initProfiler();
void            atExit(int* really);
void            displayProfilingResult();

struct ProfileBlock
{
  const char* name;
  u64         timeStart;
  u64         oldElapsedAtRoot;
  u32         index;
  u32         parentIndex;

  ProfileBlock(const char* name_)
  {
    parentIndex    = profilerParent;

    index          = profiler.count;
    name = name_;

    Profile * profile = profiler.profiles + index;
    oldElapsedAtRoot = profile->timeElapsedRoot;

    profilerParent = index;
    profiler.count++;

    timeStart      = ReadCPUTimer();
  }

  ~ProfileBlock()
  {
    u64      elapsed = ReadCPUTimer() - timeStart;
    profilerParent = parentIndex;

    Profile* profile = profiler.profiles + index;
    Profile* parent  = profiler.profiles + parentIndex;

    parent->timeElapsedChildren += elapsed;

    profile->timeElapsedRoot = oldElapsedAtRoot + elapsed;
    profile->timeElapsed += elapsed;
    profile->hitCount++;

    profile->name = name;
  }
};

#define NameConcat2(A, B)    A##B
#define NameConcat(A, B)     NameConcat2(A, B)
#define TimeBlock(blockName) ProfileBlock NameConcat(Block, __LINE__)(blockName);
#define TimeFunction         TimeBlock(__func__)

#endif
