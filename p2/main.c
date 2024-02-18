#include "./lib/common.h"
#include "./lib/json.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline f64 square(f64 a)
{
  return a * a;
}
static inline f64 radiansFromDegrees(f64 r)
{
  return r * 180.0f / M_PI;
}

#define EarthRadius 6372.8
static f64 referenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1)
{

  f64 lat1   = Y0;
  f64 lat2   = Y1;
  f64 lon1   = X0;
  f64 lon2   = X1;

  f64 dLat   = radiansFromDegrees(lat2 - lat1);
  f64 dLon   = radiansFromDegrees(lon2 - lon1);
  lat1       = radiansFromDegrees(lat1);
  lat2       = radiansFromDegrees(lat2);

  f64 a      = square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * square(sin(dLon / 2));
  f64 c      = 2.0 * asin(sqrt(a));

  f64 Result = EarthRadius * c;

  return Result;
}
int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    printf("usage: [uniform/cluster] [seed] [samples]\n");
    return 1;
  }
  bool uniform = false;
  if (strcmp(argv[1], "uniform") == 0)
  {
    uniform = true;
  }
  else if (strcmp(argv[1], "cluster") != 0)
  {
    printf("Illegal first arg, expected [uniform/cluster]");
    return 1;
  }
  srand(atoi(argv[2]));
  i64  samples = atoi(argv[3]);
  Json json;
  json.headType = JSON_OBJECT;

  initJsonObject(&json.obj);

  JsonArray array;

  JsonValue pairs;
  pairs.type = JSON_ARRAY;
  pairs.arr  = &array;

  initJsonArray(&array);

  addElementToJsonObject(&json.obj, "pairs", pairs);

  f64 haversineSum = 0;
  for (i64 i = 0; i < samples; i++)
  {
    JsonValue value;
    value.type          = JSON_OBJECT;
    JsonObject* jsonObj = (JsonObject*)malloc(sizeof(JsonObject));
    value.obj           = jsonObj;

    initJsonObject(jsonObj);

    f64       x0 = (rand() / (f32)RAND_MAX) * 180.0;
    JsonValue x0Value;
    x0Value.type   = JSON_NUMBER;
    x0Value.number = x0;
    addElementToJsonObject(jsonObj, "x0", x0Value);

    f64       y0 = (rand() / (f32)RAND_MAX) * 90.0;
    JsonValue y0Value;
    y0Value.type   = JSON_NUMBER;
    y0Value.number = y0;
    addElementToJsonObject(jsonObj, "y0", y0Value);

    f64       x1 = (rand() / (f32)RAND_MAX) * 180.0;
    JsonValue x1Value;
    x1Value.type   = JSON_NUMBER;
    x1Value.number = x1;
    addElementToJsonObject(jsonObj, "x1", x1Value);

    f64       y1 = (rand() / (f32)RAND_MAX) * 90.0;
    JsonValue y1Value;
    y1Value.type   = JSON_NUMBER;
    y1Value.number = y1;
    addElementToJsonObject(jsonObj, "y1", y1Value);

    addElementToJsonArray(&array, value);
    haversineSum += referenceHaversine(x0, y0, x1, y1);
  }
  haversineSum /= (f64)samples;

  serializeToFile(&json, "test.json");

  FILE* filePtr;
  filePtr = fopen("testSum.txt", "w");
  fprintf(filePtr, "%lf", haversineSum);
  fclose(filePtr);

  printf("Had %ld samples, sum was: %lf\n", samples, haversineSum);
}
