#include "common.h"
#include <stdio.h>

Profiler    profiler;
u32         profilerParent = 0;

static void PrintTimeElapsed(ProfileAnchor* profile, u64 TotalTSCElapsed)
{
  f64 Percent = 100.0 * ((f64)profile->timeElapsedExclusive / (f64)TotalTSCElapsed);
  printf("  %s[%lu]: %lu (%.2f%%", profile->name, profile->hitCount, profile->timeElapsedExclusive, Percent);
  if (profile->timeElapsedInclusive != profile->timeElapsedExclusive)
  {
    f64 percentWithChildren = 100.0 * ((f64)profile->timeElapsedInclusive / TotalTSCElapsed);
    printf(", %.2f%% w/children", percentWithChildren);
  }
  printf(")\n");
}
static u64 GetOSTimerFreq(void)
{
  return 1000000;
}

static u64 ReadOSTimer(void)
{
  // NOTE(casey): The "struct" keyword is not necessary here when compiling in C++,
  // but just in case anyone is using this file from C, I include it.
  struct timeval Value;
  gettimeofday(&Value, 0);

  u64 Result = GetOSTimerFreq() * (u64)Value.tv_sec + (u64)Value.tv_usec;
  return Result;
}

/* NOTE(casey): This does not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about
   static functions that aren't used. So "inline" is just the simplest way
   to tell them to stop complaining about that. */
u64 ReadCPUTimer(void)
{
  // NOTE(casey): If you were on ARM, you would need to replace __rdtsc
  // with one of their performance counter read instructions, depending
  // on which ones are available on your platform.

  return __rdtsc();
}

#define TIME_TO_WAIT 100

u64 EstimateCPUTimerFreq(void)
{
  u64 OSFreq     = GetOSTimerFreq();

  u64 CPUStart   = ReadCPUTimer();
  u64 OSStart    = ReadOSTimer();
  u64 OSElapsed  = 0;
  u64 OSEnd      = 0;
  u64 OSWaitTime = OSFreq * TIME_TO_WAIT / 1000;
  while (OSElapsed < OSWaitTime)
  {
    OSEnd     = ReadOSTimer();
    OSElapsed = OSEnd - OSStart;
  }

  u64 CPUEnd     = ReadCPUTimer();
  u64 CPUElapsed = CPUEnd - CPUStart;

  return OSFreq * CPUElapsed / OSElapsed;
}

void initProfiler()
{
  profiler.timeStart = ReadCPUTimer();
  profiler.count     = 1;
}

void displayProfilingResult()
{
  u64 endTime      = ReadCPUTimer();
  u64 totalElapsed = endTime - profiler.timeStart;
  u64 cpuFreq      = EstimateCPUTimerFreq();

  printf("\nTotal time: %0.4fms (CPU freq %lu)\n", 1000.0 * (f64)totalElapsed / (f64)cpuFreq, cpuFreq);
  for (u32 i = 0; i < 5; i++)
  {
    ProfileAnchor* profile = profiler.profiles + i;
    if (profile->timeElapsedInclusive)
    {
      PrintTimeElapsed(profile, totalElapsed);
    }
  }
}
