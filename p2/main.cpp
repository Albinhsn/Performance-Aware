#include "./lib/common.cpp"
#include "./lib/files.cpp"
#include "./lib/json.cpp"
#include "arena.h"
#include "haversine.cpp"
#include <cstdlib>
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

void parseHaversinePairs(HaversineArray* haversinePairs, Json* json)
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

  JsonArray pairsArray  = pairsValue->arr;
  haversinePairs->pairs = (HaversinePair*)malloc(sizeof(HaversinePair) * pairsArray.arraySize);
  haversinePairs->size  = pairsArray.arraySize;
  for (i32 i = 0; i < pairsArray.arraySize; i++)
  {
    JsonValue  arrayValue    = pairsArray.values[i];
    JsonObject arrayObj      = arrayValue.obj;
    haversinePairs->pairs[i] = (HaversinePair){
        .x0 = lookupJsonElement(&arrayObj, "x0")->number, //
        .y0 = lookupJsonElement(&arrayObj, "y0")->number, //
        .x1 = lookupJsonElement(&arrayObj, "x1")->number, //
        .y1 = lookupJsonElement(&arrayObj, "y1")->number, //
    };
  }
}
static inline void cleanup(HaversineArray* haversinePairs, String* fileContent)
{
  free(haversinePairs->pairs);
  free(fileContent->buffer);
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
  parseHaversinePairs(&haversinePairs, &json);
  for (u64 i = 0; i < haversinePairs.size; i++)
  {

    HaversinePair pair  = haversinePairs.pairs[i];
    f64           x0    = pair.x0;
    f64           y0    = pair.y0;
    f64           x1    = pair.x1;
    f64           y1    = pair.y1;
    f64           extra = referenceHaversine(x0, y0, x1, y1);
    sum += extra;
  }

  sum /= haversinePairs.size;
  printf("Pair count: %ld\n", haversinePairs.size);
  printf("Calculated sum: %lf\n", sum);

  printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);
  printf("%ld vs %ld\n", arena.ptr, arena.maxSize);

  // freeJson(&json);
  // cleanup(&haversinePairs, &fileContent);

  displayProfilingResult();
  return 0;
}
