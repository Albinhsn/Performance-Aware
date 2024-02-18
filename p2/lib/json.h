#ifndef JSON_H
#define JSON_H

#include "common.h"
#include <stdbool.h>
#include <stdint.h>

enum JsonType
{
  JSON_VALUE,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOL,
  JSON_NULL
};
typedef enum JsonType     JsonType;

typedef struct JsonValue  JsonValue;
typedef struct JsonValue  JsonValue;
typedef struct JsonObject JsonObject;
typedef struct JsonArray  JsonArray;

struct JsonObject
{
  char**     keys;
  JsonValue** values;
  u64        size;
  u64        cap;
};
typedef struct JsonObject JsonObject;

struct JsonArray
{
  u64 arraySize;
  u64 arrayCap;
  JsonValue* values;
};

struct JsonValue
{
  JsonType type;
  union
  {
    JsonObject obj;
    JsonArray  arr;
    bool       b;
    char*      string;
    f64 number;
  };
};
typedef struct JsonArray JsonArray;

struct Json
{
  JsonType headType;
  union
  {
    JsonValue  value;
    JsonObject obj;
    JsonArray  array;
  };
};
typedef struct Json Json;

void addElementToJsonObject(JsonObject* obj, char* key, JsonValue* value);
void                addElementToJsonArray(JsonArray* array, JsonValue value);
void                initJsonArray(JsonArray* array);
void                initJsonObject(JsonObject* obj);
bool                deserializeFromFile(Json* json, const char* filename);
bool                serializeToFile(Json* json, const char* filename);
void                debugJson(Json* json);

#endif
