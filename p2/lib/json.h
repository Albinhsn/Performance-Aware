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
typedef enum JsonType JsonType;

struct JsonValue;
struct JsonObject;
struct JsonArray;

struct JsonValue
{
  JsonType type;
  union
  {
    struct JsonObject* obj;
    struct JsonArray*  arr;
    bool               b;
    char*              string;
    float              number;
  };
};

typedef struct JsonValue JsonValue;

struct JsonObject
{
  char**     keys;
  JsonValue* values;
  u64        size;
  u64        cap;
};
typedef struct JsonObject JsonObject;

struct JsonArray
{
  uint32_t   arraySize;
  uint32_t   arrayCap;
  JsonValue* values;
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

void                addElementToJsonObject(JsonObject* obj, char* key, JsonValue value);
void                addElementToJsonArray(JsonArray* array, JsonValue value);
void                initJsonArray(JsonArray* array);
void                initJsonObject(JsonObject* obj);
bool                deserializeFromFile(Json* json, const char* filename);
bool                serializeToFile(Json* json, const char* filename);
void                debugJson(Json* json);

#endif
