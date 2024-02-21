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

#define PROFILER 1

#ifndef PROFILER
#define PROFILER 0
#endif

struct Profiler
{
  u64 StartTSC;
  u64 EndTSC;
};

extern Profiler profiler;
u64             ReadCPUTimer(void);
u64             EstimateCPUTimerFreq(void);

void            initProfiler();
void            displayProfilingResult();

#if PROFILER

struct ProfileAnchor
{
  u64         TSCElapsedExclusive; // NOTE(casey): Does NOT include children
  u64         TSCElapsedInclusive; // NOTE(casey): DOES include children
  u64         HitCount;
  char const* Label;
};
extern ProfileAnchor GlobalProfilerAnchors[4096];
extern u32           GlobalProfilerParent;

struct ProfileBlock
{
  char const* Label;
  u64         OldTSCElapsedInclusive;
  u64         StartTSC;
  u32         ParentIndex;
  u32         AnchorIndex;
  ProfileBlock(char const* Label_, u32 AnchorIndex_)
  {
    ParentIndex            = GlobalProfilerParent;

    AnchorIndex            = AnchorIndex_;
    Label                  = Label_;

    ProfileAnchor* Anchor  = GlobalProfilerAnchors + AnchorIndex;
    OldTSCElapsedInclusive = Anchor->TSCElapsedInclusive;

    GlobalProfilerParent = AnchorIndex;
    StartTSC             = ReadCPUTimer();
  }

  ~ProfileBlock(void)
  {
    u64 Elapsed           = ReadCPUTimer() - StartTSC;
    GlobalProfilerParent  = ParentIndex;

    ProfileAnchor* Parent = GlobalProfilerAnchors + ParentIndex;
    ProfileAnchor* Anchor = GlobalProfilerAnchors + AnchorIndex;

    Parent->TSCElapsedExclusive -= Elapsed;
    Anchor->TSCElapsedExclusive += Elapsed;
    Anchor->TSCElapsedInclusive = OldTSCElapsedInclusive + Elapsed;
    ++Anchor->HitCount;

    Anchor->Label = Label;
  }
};

#define NameConcat2(A, B)            A##B
#define NameConcat(A, B)             NameConcat2(A, B)
#define TimeBlock(Name)              ProfileBlock NameConcat(Block, __LINE__)(Name, __COUNTER__ + 1);
#define ProfilerEndOfCompilationUnit static_assert(__COUNTER__ < ArrayCount(GlobalProfilerAnchors), "Number of profile points exceeds size of profiler::Anchors array")
#define TimeFunction                 TimeBlock(__func__)

static void PrintTimeElapsed(u64 TotalTSCElapsed, ProfileAnchor* Anchor)
{
  f64 Percent = 100.0 * ((f64)Anchor->TSCElapsedExclusive / (f64)TotalTSCElapsed);
  printf("  %s[%lu]: %lu (%.2f%%", Anchor->Label, Anchor->HitCount, Anchor->TSCElapsedExclusive, Percent);
  if (Anchor->TSCElapsedInclusive != Anchor->TSCElapsedExclusive)
  {
    f64 PercentWithChildren = 100.0 * ((f64)Anchor->TSCElapsedInclusive / (f64)TotalTSCElapsed);
    printf(", %.2f%% w/children", PercentWithChildren);
  }
  printf(")\n");
}

static void PrintAnchorData(u64 TotalCPUElapsed)
{
  for (u32 AnchorIndex = 0; AnchorIndex < ArrayCount(GlobalProfilerAnchors); ++AnchorIndex)
  {
    ProfileAnchor* Anchor = GlobalProfilerAnchors + AnchorIndex;
    if (Anchor->TSCElapsedInclusive)
    {
      PrintTimeElapsed(TotalCPUElapsed, Anchor);
    }
  }
}
#else

#define TimeBlock(blockName)
#define TimeFunction
#endif

#endif
