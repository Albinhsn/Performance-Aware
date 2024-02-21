#include "./lib/common.cpp"
#include "./lib/files.cpp"
#include "./lib/json.cpp"
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
  TimeFunction;
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
  TimeFunction;
  free(haversinePairs->pairs);
  free(fileContent->buffer);
}

int main()
{
  initProfiler();

  Json   json;
  String fileContent;
  bool   result;

  result = ah_ReadFile(&fileContent, "./data/haversine10mil_02.json");
  if (!result)
  {
    printf("Failed to read file\n");
    return 1;
  }
  deserializeFromString(&json, fileContent);

  String string;
  bool   res = ah_ReadFile(&string, "./data/haversine10milSum_02.txt");
  if (!res)
  {
    printf("Failed to read file\n");
    return 1;
  }

  f64            expected = strtod(string.buffer, NULL);

  f64            sum      = 0;

  HaversineArray haversinePairs;
  parseHaversinePairs(&haversinePairs, &json);
  {
    TimeBlock("Sum");
    for (u64 i = 0; i < haversinePairs.size; i++)
    {

      HaversinePair pair = haversinePairs.pairs[i];
      f64           x0   = pair.x0;
      f64           y0   = pair.y0;
      f64           x1   = pair.x1;
      f64           y1   = pair.y1;
      sum += referenceHaversine(x0, y0, x1, y1);
    }
  }
  sum /= haversinePairs.size;
  printf("Pair count: %ld\n", haversinePairs.size);
  printf("Calculated sum: %lf\n", sum);

  printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);

  freeJson(&json);
  cleanup(&haversinePairs, &fileContent);

  displayProfilingResult();
  return 0;
}
