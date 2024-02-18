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

static inline f64 clampClusterValue(f64 x, f64 lower, f64 upper)
{
  x = x < lower ? lower : x;
  x = x > upper ? upper : x;
  return x;
}

static inline f64 generateRandomXCoordinate(f64 offset)
{
  f64 xMin = -180 + offset;
  f64 xMax = 180 - offset;
  f64 x    = (rand() / (f32)RAND_MAX) * 360.0 - 180.0;
  x        = x < xMin ? xMin : x;
  x        = x > xMax ? xMax : x;
  return x;
}
static inline f64 generateRandomYCoordinate(f64 offset)
{
  f64 yMin = -90 + offset;
  f64 yMax = 90 - offset;
  f64 y    = (rand() / (f32)RAND_MAX) * 180.0 - 90.0;
  y        = y < yMin ? yMin : y;
  y        = y > yMax ? yMax : y;
  return y;
}

static inline f64 createAndAddRandomClusterValue(JsonObject* jsonObj, char* name, f64 bound, f64 offset)
{
  f64       v = (rand() / (f32)RAND_MAX) * bound * 2 - bound + offset;
  JsonValue value;
  value.type   = JSON_NUMBER;
  value.number = v;
  addElementToJsonObject(jsonObj, name, value);
  return v;
}

static f64 addClusterCoordinates(JsonArray* array, i64 size)
{
  f64 yOffset  = 0;
  f64 xOffset  = 0;

  f64 x0Offset = generateRandomXCoordinate(xOffset);
  f64 x1Offset = generateRandomXCoordinate(xOffset);
  f64 y0Offset = generateRandomYCoordinate(yOffset);
  f64 y1Offset = generateRandomYCoordinate(yOffset);

  f64 sum      = 0;
  for (i64 i = 0; i < size; i++)
  {
    JsonValue value;
    value.type          = JSON_OBJECT;
    JsonObject* jsonObj = (JsonObject*)malloc(sizeof(JsonObject));
    value.obj           = jsonObj;

    initJsonObject(jsonObj);

    f64 x0 = createAndAddRandomClusterValue(jsonObj, "x0", xOffset, x0Offset);
    f64 y0 = createAndAddRandomClusterValue(jsonObj, "y0", yOffset, y0Offset);
    f64 x1 = createAndAddRandomClusterValue(jsonObj, "x1", xOffset, x1Offset);
    f64 y1 = createAndAddRandomClusterValue(jsonObj, "y1", yOffset, y1Offset);

    sum += referenceHaversine(x0, y0, x1, y1);
  }

  return sum;
}

static inline f64 createAndAddUniformClusterValue(JsonObject* obj, char* name, bool x)
{

  f64       v = x ? generateRandomXCoordinate(0) : generateRandomYCoordinate(0);
  JsonValue value;
  value.type   = JSON_NUMBER;
  value.number = v;
  addElementToJsonObject(obj, name, value);

  return v;
}

static f64 addUniformCoordinates(JsonArray* array)
{

  JsonValue value;
  value.type          = JSON_OBJECT;
  JsonObject* jsonObj = (JsonObject*)malloc(sizeof(JsonObject));
  value.obj           = jsonObj;

  initJsonObject(jsonObj);

  f64 x0 = createAndAddUniformClusterValue(jsonObj, "x0", true);
  f64 y0 = createAndAddUniformClusterValue(jsonObj, "y0", false);
  f64 x1 = createAndAddUniformClusterValue(jsonObj, "x1", true);
  f64 y1 = createAndAddUniformClusterValue(jsonObj, "y1", false);

  addElementToJsonArray(array, value);
  return referenceHaversine(x0, y0, x1, y1);
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
  if (uniform)
  {
    for (i64 i = 0; i < samples; i++)
    {
      haversineSum += addUniformCoordinates(&array);
    }
  }
  else
  {
    i32 clusters    = 10;
    i32 clusterSize = samples / clusters;
    for (i64 i = 0; i < clusters; i++)
    {
      f64 extra = addClusterCoordinates(&array, clusterSize);
      haversineSum += extra;
    }
    for (i64 i = 0; i < samples % clusters; i++)
    {
      haversineSum += addUniformCoordinates(&array);
    }
  }
  haversineSum /= (f64)samples;

  serializeToFile(&json, "test.json");

  FILE* filePtr;
  filePtr = fopen("testSum.txt", "w");
  fprintf(filePtr, "%lf", haversineSum);
  fclose(filePtr);

  printf("Had %ld samples, sum was: %lf\n", samples, haversineSum);
}
