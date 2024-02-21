#ifndef JSON_H
#define JSON_H

#include "common.h"
#include "string.h"
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

typedef struct JsonValue  JsonValue;
typedef struct JsonObject JsonObject;
typedef struct JsonArray  JsonArray;

struct JsonObject
{
  String*    keys;
  JsonValue* values;
  u64        size;
  u64        cap;
};

struct JsonArray
{
  u64        arraySize;
  u64        arrayCap;
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
    String     string;
    f64        number;
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

void                addElementToJsonObject(JsonObject* obj, char* key, JsonValue* value);
void                addElementToJsonArray(JsonArray* array, JsonValue value);
void                initJsonArray(JsonArray* array);
void                initJsonObject(JsonObject* obj);
bool                deserializeFromString(Json* json, String fileContent);
bool                serializeToFile(Json* json, const char* filename);
void                debugJson(Json* json);

JsonValue*          lookupJsonElement(JsonObject* obj, const char* key);
void                freeJsonObject(JsonObject* obj);
void                freeJsonValue(JsonValue* value);
void                freeJsonArray(JsonArray* array);
void                freeJson(Json* json);
#endif
