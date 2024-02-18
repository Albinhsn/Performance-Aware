#include "./lib/files.h"
#include "./lib/json.h"
#include "haversine.h"

int main()
{
  Json json;
  deserializeFromFile(&json, "./data/haversine10mil_02.json");
  String string;
  bool   res = ah_ReadFile(&string, "./data/haversine10milSum_02.txt");
  if (!res)
  {
    printf("Failed to read file\n");
    return 1;
  }

  f64 expected = strtod(string.buffer, NULL);
  printf("Got expected %lf\n", expected);

  f64       sum   = 0;
  JsonArray array = json.obj.values[0]->arr;
  u64       size  = array.arraySize;
  printf("Size was %ld\n", size);

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
  printf("Pair count: %ld\n", size);
  printf("Calculated sum: %lf\n", sum);

  printf("Expected %lf\n", expected);
  printf("Difference %lf\n", expected - sum);
}
