#include "./lib/common.h"
#include "./lib/files.h"
#include "./lib/json.h"
#include "haversine.h"

void solve(u64 cpuFreq)
{
}

static void PrintTimeElapsed(char const* Label, u64 TotalTSCElapsed, u64 Begin, u64 End)
{
  u64 Elapsed = End - Begin;
  f64 Percent = 100.0 * ((f64)Elapsed / (f64)TotalTSCElapsed);
  printf("  %s: %lu (%.2f%%)\n", Label, Elapsed, Percent);
}

int main()
{
  u64    Prof_Begin      = 0;
  u64    Prof_Read       = 0;
  u64    Prof_MiscSetup  = 0;
  u64    Prof_Parse      = 0;
  u64    Prof_Sum        = 0;
  u64    Prof_MiscOutput = 0;
  u64    Prof_End        = 0;

  Json   json;
  String fileContent;
  bool   result;

  Prof_Begin = ReadCPUTimer();
  result     = ah_ReadFile(&fileContent, "./data/haversine10mil_02.json");
  if (!result)
  {
    printf("Failed to read file\n");
    return false;
  }
  Prof_Read = ReadCPUTimer();
  deserializeFromFile(&json, fileContent);
  Prof_Parse = ReadCPUTimer();

  String string;
  bool   res = ah_ReadFile(&string, "./data/haversine10milSum_02.txt");
  if (!res)
  {
    printf("Failed to read file\n");
    return 1;
  }

  f64 expected = strtod(string.buffer, NULL);
  // printf("Got expected %lf\n", expected);

  f64        sum    = 0;
  JsonArray  array  = json.obj.values[0]->arr;
  u64        size   = array.arraySize;
  JsonValue* values = array.values;
  // printf("Size was %ld\n", size);
  Prof_MiscSetup = ReadCPUTimer();
  for (u64 i = 0; i < size; i++)
  {
    JsonObject obj = values[i].obj;

    f64        x0  = obj.values[0]->number;
    f64        y0  = obj.values[1]->number;
    f64        x1  = obj.values[2]->number;
    f64        y1  = obj.values[3]->number;
    sum += referenceHaversine(x0, y0, x1, y1);
  }
  sum /= size;
  Prof_Sum = ReadCPUTimer();
  printf("Pair count: %ld\n", size);
  printf("Calculated sum: %lf\n", sum);

  printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);
  Prof_End            = ReadCPUTimer();

  u64 TotalCPUElapsed = Prof_End - Prof_Begin;

  u64 CPUFreq         = EstimateCPUTimerFreq();
  if (CPUFreq)
  {
    printf("\nTotal time: %0.4fms (CPU freq %lu)\n", 1000.0 * (f64)TotalCPUElapsed / (f64)CPUFreq, CPUFreq);
  }

  PrintTimeElapsed("Reading", TotalCPUElapsed, Prof_Begin, Prof_Read);
  PrintTimeElapsed("Parsing", TotalCPUElapsed, Prof_Read, Prof_Parse);
  PrintTimeElapsed("MiscSetup", TotalCPUElapsed, Prof_Parse, Prof_MiscSetup);
  PrintTimeElapsed("Sum", TotalCPUElapsed, Prof_MiscSetup, Prof_Sum);
  PrintTimeElapsed("MiscOutput", TotalCPUElapsed, Prof_Sum, Prof_End);

  return 0;
}
