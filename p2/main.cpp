#include "./lib/common.cpp"
#include "./lib/files.cpp"
#include "./lib/json.cpp"
#include "arena.cpp"
#include "arena.h"
#include "haversine.cpp"
#include <cstdlib>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

struct HaversinePair
{
  f64 x0, y0;
  f64 x1, y1;
};
struct HaversineArray
{
  HaversinePair* pairs;
  u64            size;
};

struct ParseHaversinePairsArgs
{
  HaversinePair* pairs;
  JsonArray*     jsonArray;
  u64            start;
  u64            end;
};

void* gatherParseHaversinePairs(void* arg)
{
  ParseHaversinePairsArgs* args           = (ParseHaversinePairsArgs*)arg;
  JsonValue*               values         = args->jsonArray->values;
  HaversinePair*           haversinePairs = args->pairs;
  for (i32 i = args->start; i < args->end; i++)
  {
    JsonValue  arrayValue = values[i];
    JsonObject arrayObj   = arrayValue.obj;
    haversinePairs[i]     = (HaversinePair){
            .x0 = lookupJsonElement(&arrayObj, "x0")->number, //
            .y0 = lookupJsonElement(&arrayObj, "y0")->number, //
            .x1 = lookupJsonElement(&arrayObj, "x1")->number, //
            .y1 = lookupJsonElement(&arrayObj, "y1")->number, //
    };
  }

  return 0;
}
struct HaversineThreadArgs
{
  HaversineArray* array;
  f64*            sums;
  u64             start;
  u64             end;
};

void* gatherHaversinePairSum(void* arg)
{
  HaversineThreadArgs* args  = (HaversineThreadArgs*)arg;
  f64                  sum   = 0.0f;
  HaversinePair*       pairs = args->array->pairs;
  for (u64 i = args->start; i < args->end; i++)
  {
    HaversinePair pair  = pairs[i];
    f64           x0    = pair.x0;
    f64           y0    = pair.y0;
    f64           x1    = pair.x1;
    f64           y1    = pair.y1;
    f64           extra = referenceHaversine(x0, y0, x1, y1);
    sum += extra;
  }
  *args->sums = sum;
  return 0;
}

void parseHaversinePairs(Arena* arena, HaversineArray* haversinePairs, Json* json)
{
  if (json->headType != JSON_OBJECT)
  {
    printf("Head wasn't object?\n");
    exit(1);
  }

  JsonValue* pairsValue = lookupJsonElement(&json->obj, "pairs");
  if (!pairsValue || pairsValue->type != JSON_ARRAY)
  {
    printf("Couldn't find pairs in object or it wasn't array\n");
    exit(1);
  }

  JsonArray pairsArray                = pairsValue->arr;
  haversinePairs->pairs               = ArenaPushArray(arena, HaversinePair, pairsArray.arraySize);
  haversinePairs->size                = pairsArray.arraySize;

  u64                     threadCount = 10;
  u64                     step        = haversinePairs->size / threadCount;
  pthread_t               threadIds[threadCount];
  ParseHaversinePairsArgs args[threadCount];
  f64                     threadSums[threadCount];
  for (i32 i = 0; i < threadCount; i++)
  {
    args[i].start     = step * i;
    args[i].end       = step * (i + 1);
    args[i].pairs     = haversinePairs->pairs;
    args[i].jsonArray = &pairsArray;
  }
  args[threadCount - 1].end = haversinePairs->size;
  for (i32 i = 0; i < threadCount; i++)
  {
    pthread_create(&threadIds[i], NULL, gatherParseHaversinePairs, (void*)&args[i]);
  }

  for (i32 i = 0; i < threadCount; i++)
  {
    if (pthread_join(threadIds[i], NULL) != 0)
    {
      printf("Failed to join?\n");
      exit(2);
    }
  }
}
static inline void cleanup(String* string, String* fileContent)
{
  free(fileContent->buffer);
  free(string->buffer);
}

int main()
{
  initProfiler();
  u64   stackSize   = ((u64)(1024 * 1024 * 1024)) * 3;
  u8*   stackMemory = (u8*)malloc(sizeof(u8) * stackSize);
  Arena arena;
  arena.ptr     = 0;
  arena.memory  = (u64)stackMemory;
  arena.maxSize = stackSize;

  Json   json;
  String fileContent;
  bool   result;

  result = ah_ReadFile(&fileContent, "./data/haversine10mil_03.json");
  if (!result)
  {
    printf("Failed to read file\n");
    return 1;
  }
  deserializeFromString(&json, &arena, fileContent);

  String string;
  bool   res = ah_ReadFile(&string, "./data/haversine10milSum_03.txt");
  if (!res)
  {
    printf("Failed to read file\n");
    return 1;
  }

  f64            expected = strtod((char*)string.buffer, NULL);
  f64            sum      = 0;

  HaversineArray haversinePairs;
  parseHaversinePairs(&arena, &haversinePairs, &json);

  u64                 threadCount = 10;
  u64                 step        = haversinePairs.size / threadCount;
  pthread_t           threadIds[threadCount];
  HaversineThreadArgs args[threadCount];
  f64                 threadSums[threadCount];
  for (i32 i = 0; i < threadCount; i++)
  {
    args[i].start = step * i;
    args[i].end   = step * (i + 1);
    args[i].array = &haversinePairs;
    args[i].sums  = &threadSums[i];
  }
  args[threadCount - 1].end = haversinePairs.size;
  for (i32 i = 0; i < threadCount; i++)
  {
    pthread_create(&threadIds[i], NULL, gatherHaversinePairSum, (void*)&args[i]);
  }

  for (i32 i = 0; i < threadCount; i++)
  {
    if (pthread_join(threadIds[i], NULL) != 0)
    {
      printf("Failed to join?\n");
      exit(2);
    }
    sum += threadSums[i];
  }

  sum /= haversinePairs.size;
  printf("Pair count: %ld\n", haversinePairs.size);
  printf("Calculated sum: %lf\n", sum);

  printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);
  printf("Used %ld of %ld\n", arena.ptr, arena.maxSize);

  cleanup(&string, &fileContent);
  free((void*)arena.memory);

  displayProfilingResult();
  return 0;
}
