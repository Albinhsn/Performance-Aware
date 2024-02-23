#include "../p2//lib/files.h"
#include "../p2/lib/string.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" void NOPAllBytesASM(u64 Count);
extern "C" void NOP1x3AllBytesASM(u64 Count);
extern "C" void NOP1x9AllBytesASM(u64 Count);

enum TestMode {
  TestMode_Uninitialized,
  TestMode_Testing,
  TestMode_Completed,
  TestMode_Error
};
enum AllocationMode {
  AllocationMode_Malloc,
  AllocationMode_None,
  AllocationMode_Count
};

struct RepetitionTestResults {
  u64 testCount;
  u64 totalTime;
  u64 maxTime;
  u64 minTime;
};

struct TestFuncParameters {
  String dest;
  const char *filename;
  AllocationMode allocationMode;
};

struct RepetitionTester {
  u64 targetProcessedByteCount;
  u64 CPUTimerFreq;
  u64 tryForTime;
  u64 testsStartedAt;

  TestMode mode;
  bool printNewMinimums;
  u32 openBlockCount;
  u32 closeBlockCount;
  u64 timeAccumulatedOnThisTest;
  u64 bytesAccumulatedOnThisTest;

  RepetitionTestResults results;
};

static f64 SecondsFromCPUTime(f64 CPUTime, u64 CPUTimerFreq) {
  f64 Result = 0.0;
  if (CPUTimerFreq) {
    Result = (CPUTime / (f64)CPUTimerFreq);
  }

  return Result;
}

static void PrintTime(char const *Label, f64 CPUTime, u64 CPUTimerFreq,
                      u64 ByteCount) {
  printf("%s: %.0f", Label, CPUTime);
  if (CPUTimerFreq) {
    f64 Seconds = SecondsFromCPUTime(CPUTime, CPUTimerFreq);
    printf(" (%fms)", 1000.0f * Seconds);

    if (ByteCount) {
      f64 Gigabyte = (1024.0f * 1024.0f * 1024.0f);
      f64 BestBandwidth = ByteCount / (Gigabyte * Seconds);
      printf(" %fgb/s", BestBandwidth);
    }
  }
}

static void PrintTime(char const *Label, u64 CPUTime, u64 CPUTimerFreq,
                      u64 ByteCount) {
  PrintTime(Label, (f64)CPUTime, CPUTimerFreq, ByteCount);
}

static void PrintResults(RepetitionTestResults Results, u64 CPUTimerFreq,
                         u64 ByteCount) {
  PrintTime("Min", (f64)Results.minTime, CPUTimerFreq, ByteCount);
  printf("\n");

  PrintTime("Max", (f64)Results.maxTime, CPUTimerFreq, ByteCount);
  printf("\n");

  if (Results.testCount) {
    PrintTime("Avg", (f64)Results.totalTime / (f64)Results.testCount,
              CPUTimerFreq, ByteCount);
    printf("\n");
  }
}
static void Error(RepetitionTester *tester, char const *Message) {
  tester->mode = TestMode_Error;
  fprintf(stderr, "ERROR: %s\n", Message);
}

static void NewTestWave(RepetitionTester *tester, u64 targetProcessedByteCount,
                        u64 CPUTimerFreq, u32 SecondsToTry = 10) {
  if (tester->mode == TestMode_Uninitialized) {
    tester->mode = TestMode_Testing;
    tester->targetProcessedByteCount = targetProcessedByteCount;
    tester->CPUTimerFreq = CPUTimerFreq;
    tester->printNewMinimums = true;
    tester->results.minTime = (u64)INT64_MAX;
  } else if (tester->mode == TestMode_Completed) {
    tester->mode = TestMode_Testing;

    if (tester->targetProcessedByteCount != targetProcessedByteCount) {
      Error(tester, "TargetProcessedByteCount changed");
    }

    if (tester->CPUTimerFreq != CPUTimerFreq) {
      Error(tester, "CPU frequency changed");
    }
  }

  tester->tryForTime = SecondsToTry * CPUTimerFreq;
  tester->testsStartedAt = ReadCPUTimer();
}

static void BeginTime(RepetitionTester *tester) {
  ++tester->openBlockCount;
  tester->timeAccumulatedOnThisTest -= ReadCPUTimer();
}

static void EndTime(RepetitionTester *tester) {
  ++tester->closeBlockCount;
  tester->timeAccumulatedOnThisTest += ReadCPUTimer();
}

static void CountBytes(RepetitionTester *tester, u64 ByteCount) {
  tester->bytesAccumulatedOnThisTest += ByteCount;
}

static bool IsTesting(RepetitionTester *tester) {
  if (tester->mode == TestMode_Testing) {
    u64 CurrentTime = ReadCPUTimer();

    if (tester->openBlockCount) // NOTE(casey): We don't count tests that had no
                                // timing blocks - we assume they took some
                                // other path
    {
      if (tester->openBlockCount != tester->closeBlockCount) {
        Error(tester, "Unbalanced BeginTime/EndTime");
      }

      if (tester->bytesAccumulatedOnThisTest !=
          tester->targetProcessedByteCount) {
        Error(tester, "Processed byte count mismatch");
      }

      if (tester->mode == TestMode_Testing) {
        RepetitionTestResults *results = &tester->results;
        u64 ElapsedTime = tester->timeAccumulatedOnThisTest;
        results->testCount += 1;
        results->totalTime += ElapsedTime;
        if (results->maxTime < ElapsedTime) {
          results->maxTime = ElapsedTime;
        }

        if (results->minTime > ElapsedTime) {
          results->minTime = ElapsedTime;

          // NOTE(casey): Whenever we get a new minimum time, we reset the clock
          // to the full trial time
          tester->testsStartedAt = CurrentTime;

          if (tester->printNewMinimums) {
            PrintTime("Min", results->minTime, tester->CPUTimerFreq,
                      tester->bytesAccumulatedOnThisTest);
            printf("               \r");
            fflush(stdout);
          }
        }

        tester->openBlockCount = 0;
        tester->closeBlockCount = 0;
        tester->timeAccumulatedOnThisTest = 0;
        tester->bytesAccumulatedOnThisTest = 0;
      }
    }

    if ((CurrentTime - tester->testsStartedAt) > tester->tryForTime) {
      tester->mode = TestMode_Completed;

      printf("                                                          \r");
      PrintResults(tester->results, tester->CPUTimerFreq,
                   tester->targetProcessedByteCount);
    }
  }

  bool Result = (tester->mode == TestMode_Testing);
  return Result;
}

typedef void TestFunc(u64 count);

struct TestFunction {
  char const *Name;
  TestFunc *Func;
};
TestFunction testFunctions[] = {
    {"NOPAllBytes", NOPAllBytesASM},
    {"NOP1x3AllBytes", NOP1x3AllBytesASM},
    {"NOP1x9AllBytes", NOP1x9AllBytesASM},
};

int main() {

  u64 CPUTimerFreq = EstimateCPUTimerFreq();
  struct stat Stat;
  stat("./data/haversine10mil_02.json", &Stat);

  String dest = (String){.len = (u64)Stat.st_size, .buffer = NULL};
  dest.buffer = (u8 *)malloc(sizeof(u8) * Stat.st_size);

  RepetitionTester testers[ArrayCount(testFunctions)] = {};

  for (;;) {
    for (u32 funcIndex = 0; funcIndex < ArrayCount(testFunctions);
         ++funcIndex) {
      RepetitionTester *tester = &testers[funcIndex];
      TestFunction testFunc = testFunctions[funcIndex];

      printf("\n--- %s ---\n", testFunc.Name);
      NewTestWave(tester, dest.len, CPUTimerFreq);
      while (IsTesting(tester)) {
        BeginTime(tester);
        NOPAllBytesASM(dest.len);
        EndTime(tester);
        CountBytes(tester, dest.len);
      }
    }
  }

  return 0;
}
