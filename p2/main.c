#include "./lib/common.h"
#include "./lib/files.h"
#include "./lib/json.h"
#include "haversine.h"

void solve()
{
  Json json;
  deserializeFromFile(&json, "./data/haversine10mil_02.json");
  String string;
  bool   res = ah_ReadFile(&string, "./data/haversine10milSum_02.txt");
  if (!res)
  {
    printf("Failed to read file\n");
    return;
  }

  f64 expected = strtod(string.buffer, NULL);
  // printf("Got expected %lf\n", expected);

  f64       sum   = 0;
  JsonArray array = json.obj.values[0]->arr;
  u64       size  = array.arraySize;
  // printf("Size was %ld\n", size);

  for (u64 i = 0; i < size; i++)
  {
    JsonObject obj = array.values[i].obj;

    f64        x0  = obj.values[0]->number;
    f64        y0  = obj.values[1]->number;
    f64        x1  = obj.values[2]->number;
    f64        y1  = obj.values[3]->number;
    sum += referenceHaversine(x0, y0, x1, y1);
  }
  sum /= size;
  // printf("Pair count: %ld\n", size);
  // printf("Calculated sum: %lf\n", sum);

  // printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);
}

int main()
{
  u64 OSFreq = GetOSTimerFreq();
  printf("    OS Freq: %lu\n", OSFreq);

  u64 CPUStart  = ReadCPUTimer();
  u64 OSStart   = ReadOSTimer();
  u64 OSEnd     = 0;
  u64 OSElapsed = 0;
  OSEnd         = ReadOSTimer();
  solve();

  u64 CPUEnd     = ReadCPUTimer();
  u64 CPUElapsed = CPUEnd - CPUStart;

  printf("   OS Timer: %lu -> %lu = %lu elapsed\n", OSStart, OSEnd, OSElapsed);
  printf(" OS Seconds: %.4f\n", (f64)OSElapsed / (f64)OSFreq);

  printf("  CPU Timer: %lu -> %lu = %lu elapsed\n", CPUStart, CPUEnd, CPUElapsed);

  return 0;
}
