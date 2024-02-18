#include "json.h"
#include "../haversine.h"
#include "common.h"
#include "files.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADVANCE(curr) ((*curr)++)

struct Buffer
{
  u8* buffer;
  u64 curr;
  u64 len;
};
typedef struct Buffer Buffer;

static inline u8      getCurrentCharBuffer(Buffer* buffer)
{
  return buffer->buffer[buffer->curr];
}

static inline void advanceBuffer(Buffer* buffer)
{
  buffer->curr++;
}

void debugJsonObject(JsonObject* object);
void debugJsonArray(JsonArray* arr);

void debugJsonValue(JsonValue* value)
{
  switch (value->type)
  {
  case JSON_OBJECT:
  {
    debugJsonObject(&value->obj);
    break;
  }
  case JSON_BOOL:
  {
    if (value->b)
    {
      printf("true");
    }
    else
    {
      printf("false");
    }
    break;
  }
  case JSON_NULL:
  {
    printf("null");
    break;
  }
  case JSON_NUMBER:
  {
    printf("%lf", value->number);
    break;
  }
  case JSON_ARRAY:
  {
    debugJsonArray(&value->arr);
    break;
  }
  case JSON_STRING:
  {
    printf("\"%s\"", value->string);
    break;
  }
  default:
  {
    break;
  }
  }
}

void debugJsonObject(JsonObject* object)
{
  printf("{\n");
  for (i32 i = 0; i < object->size; i++)
  {
    printf("\"%s\":", object->keys[i]);
    debugJsonValue(object->values[i]);
    if (i != object->size - 1)
    {
      printf(",\n");
    }
  }
  printf("\n}n");
}

void debugJsonArray(JsonArray* arr)
{
  printf("[");
  for (i32 i = 0; i < arr->arraySize; i++)
  {
    debugJsonValue(&arr->values[i]);
    if (i != arr->arraySize - 1)
    {
      printf(", ");
    }
  }
  printf("]");
}

void debugJson(Json* json)
{
  switch (json->headType)
  {
  case JSON_OBJECT:
  {
    debugJsonObject(&json->obj);
    break;
  }
  case JSON_ARRAY:
  {
    debugJsonArray(&json->array);
    break;
  }
  case JSON_VALUE:
  {
    debugJsonValue(&json->value);
    break;
  }
  default:
  {
    printf("HOW IS THIS THE HEAD TYPE? %d\n", json->headType);
    break;
  }
  }
  printf("\n");
}

inline void resizeObject(JsonObject* obj)
{
  if (obj->size >= obj->cap)
  {
    obj->cap *= 2;
    obj->values = realloc(obj->values, obj->cap * sizeof(JsonValue));
    obj->keys   = realloc(obj->keys, obj->cap * sizeof(char*));
    for (i32 i = obj->size; i < obj->cap; i++)
    {
      obj->values[i] = (JsonValue*)malloc(sizeof(JsonValue));
    }
  }
}

inline void resizeArray(JsonArray* arr)
{
  if (arr->arraySize == 0)
  {
    arr->arrayCap = 4;
    arr->values   = (JsonValue*)malloc(sizeof(JsonValue) * arr->arrayCap);
  }
  else if (arr->arraySize >= arr->arrayCap)
  {
    arr->arrayCap *= 2;
    arr->values = realloc(arr->values, arr->arrayCap * sizeof(JsonValue));
  }
}

void addElementToJsonObject(JsonObject* obj, char* key, JsonValue* value)
{

  resizeObject(obj);
  obj->values[obj->size] = value;
  obj->keys[obj->size]   = key;
  obj->size++;
}
void addElementToJsonArray(JsonArray* array, JsonValue value)
{
  resizeArray(array);
  array->values[array->arraySize++] = value;
}
void initJsonArray(JsonArray* array)
{
  array->arraySize = 0;
  array->arrayCap  = 4;
  array->values    = (JsonValue*)malloc(sizeof(JsonValue) * array->arrayCap);
}
void initJsonObject(JsonObject* obj)
{
  obj->size   = 0;
  obj->cap    = 4;
  obj->values = (JsonValue**)malloc(sizeof(JsonValue*) * obj->cap);
  obj->keys   = (char**)malloc(sizeof(char*) * obj->cap);
  for (i32 i = obj->size; i < obj->cap; i++)
  {
    obj->values[i] = (JsonValue*)malloc(sizeof(JsonValue));
  }
}

void serializeJsonValue(JsonValue* value, FILE* filePtr);
void serializeJsonObject(JsonObject* object, FILE* filePtr);

void serializeJsonArray(JsonArray* arr, FILE* filePtr)
{
  fwrite("[", 1, 1, filePtr);
  for (i32 i = 0; i < arr->arraySize; i++)
  {
    serializeJsonValue(&arr->values[i], filePtr);
    if (i != arr->arraySize - 1)
    {
      fwrite(",", 1, 1, filePtr);
    }
  }
  fwrite("]", 1, 1, filePtr);
}

void serializeJsonObject(JsonObject* object, FILE* filePtr)
{
  fwrite("{", 1, 1, filePtr);
  for (i32 i = 0; i < object->size; i++)
  {
    fprintf(filePtr, "\"%s\":", object->keys[i]);
    serializeJsonValue(object->values[i], filePtr);
    if (i != object->size - 1)
    {
      fwrite(",", 1, 1, filePtr);
    }
  }
  fwrite("}", 1, 1, filePtr);
}

void serializeJsonValue(JsonValue* value, FILE* filePtr)
{
  switch (value->type)
  {
  case JSON_OBJECT:
  {
    serializeJsonObject(&value->obj, filePtr);
    break;
  }
  case JSON_BOOL:
  {
    if (value->b)
    {
      fwrite("true", 1, 4, filePtr);
    }
    else
    {
      fwrite("false", 1, 5, filePtr);
    }
    break;
  }
  case JSON_NULL:
  {
    fwrite("null", 1, 4, filePtr);
    break;
  }
  case JSON_NUMBER:
  {
    u64 converted = convertMe(value->number);
    fprintf(filePtr, "%ld", converted);
    break;
  }
  case JSON_ARRAY:
  {
    serializeJsonArray(&value->arr, filePtr);
    break;
  }
  case JSON_STRING:
  {
    fprintf(filePtr, "\"%s\"", value->string);
    break;
  }
  default:
  {
    break;
  }
  }
}

bool serializeToFile(Json* json, const char* filename)
{
  FILE* filePtr;

  filePtr = fopen(filename, "w");
  if (!filePtr)
  {
    printf("Failed to open '%s'\n", filename);
    return false;
  }
  switch (json->headType)
  {
  case JSON_OBJECT:
  {
    serializeJsonObject(&json->obj, filePtr);
    break;
  }
  case JSON_ARRAY:
  {
    serializeJsonArray(&json->array, filePtr);
    break;
  }
  case JSON_VALUE:
  {
    serializeJsonValue(&json->value, filePtr);
    break;
  }
  default:
  {
    printf("HOW IS THIS THE HEAD TYPE? %d\n", json->headType);
    break;
  }
  }
  fclose(filePtr);
  return true;
}

f64 parseNumber(Buffer* buffer)
{
  u64 start = buffer->curr;
  advanceBuffer(buffer);
  while (isdigit(getCurrentCharBuffer(buffer)))
  {
    advanceBuffer(buffer);
  }
  if (getCurrentCharBuffer(buffer) == '.')
  {
    advanceBuffer(buffer);
    while (isdigit(getCurrentCharBuffer(buffer)))
    {
      advanceBuffer(buffer);
    }
  }

  u64  size                    = buffer->curr - start;
  char prev                    = buffer->buffer[start + size];
  buffer->buffer[start + size] = '\0';

  u64 res                      = strtoul(&buffer->buffer[start], NULL, 10);
  buffer->buffer[start + size] = prev;

  return *(f64*)&res;
}
bool parseString(char** key, Buffer* buffer)
{
  advanceBuffer(buffer);
  u64 start = buffer->curr;
  while (getCurrentCharBuffer(buffer) != '"')
  {
    advanceBuffer(buffer);
  }
  u64 len     = buffer->curr - start;
  *key        = (char*)malloc(sizeof(char) * (1 + len));
  (*key)[len] = '\0';
  strncpy(*key, &buffer->buffer[start], len);
  advanceBuffer(buffer);

  return true;
}

static inline void skipWhitespace(Buffer* buffer)
{
  while (getCurrentCharBuffer(buffer) == ' ' || getCurrentCharBuffer(buffer) == '\n')
  {
    advanceBuffer(buffer);
  }
}

bool consumeToken(Buffer* buffer, char expected)
{
  if (expected != getCurrentCharBuffer(buffer))
  {
    printf("Expected '%c' but got '%c'\n", expected, getCurrentCharBuffer(buffer));
    return false;
  }
  advanceBuffer(buffer);
  return true;
}

bool parseJsonValue(JsonValue* value, Buffer* buffer);
bool parseJsonArray(JsonArray* arr, Buffer* buffer);

bool parseKeyValuePair(JsonObject* obj, Buffer* buffer)
{
  resizeObject(obj);

  bool res = parseString(&obj->keys[obj->size], buffer);
  if (!res)
  {
    return false;
  }
  skipWhitespace(buffer);

  if (!consumeToken(buffer, ':'))
  {
    return false;
  }

  res = parseJsonValue(obj->values[obj->size], buffer);
  if (!res)
  {
    return false;
  }
  obj->size++;

  skipWhitespace(buffer);
  return true;
}

bool parseJsonObject(JsonObject* obj, Buffer* buffer)
{
  advanceBuffer(buffer);
  skipWhitespace(buffer);

  // end or string
  while (getCurrentCharBuffer(buffer) != '}')
  {
    bool res = parseKeyValuePair(obj, buffer);
    if (!res)
    {
      printf("Failed to parse key value pair\n");
      return false;
    }

    skipWhitespace(buffer);
    if (getCurrentCharBuffer(buffer) == ',')
    {
      advanceBuffer(buffer);
    }
    skipWhitespace(buffer);
  }
  advanceBuffer(buffer);
  return true;
}

bool parseJsonArray(JsonArray* arr, Buffer* buffer)
{
  advanceBuffer(buffer);
  skipWhitespace(buffer);
  bool res;
  while (getCurrentCharBuffer(buffer) != ']')
  {
    resizeArray(arr);
    res = parseJsonValue(&arr->values[arr->arraySize], buffer);
    if (!res)
    {
      return false;
    }
    arr->arraySize++;
    skipWhitespace(buffer);
    if (getCurrentCharBuffer(buffer) == ',')
    {
      advanceBuffer(buffer);
    }
  }
  advanceBuffer(buffer);

  return true;
}
bool parseKeyword(Buffer* buffer, char* expected, u8 len)
{
  for (i32 i = 0; i < len; i++)
  {
    if (buffer->buffer[buffer->curr + 1 + i] != expected[i])
    {
      return false;
    }
  }
  return true;
}

bool parseJsonValue(JsonValue* value, Buffer* buffer)
{
  char currentChar = getCurrentCharBuffer(buffer);
  if (isdigit(currentChar) || currentChar == '-')
  {
    value->type   = JSON_NUMBER;
    value->number = parseNumber(buffer);
    return true;
  }

  switch (currentChar)
  {
  case '\"':
  {
    value->type = JSON_STRING;
    return parseString(&value->string, buffer);
  }
  case '{':
  {
    value->type = JSON_OBJECT;
    initJsonObject(&value->obj);
    return parseJsonObject(&value->obj, buffer);
  }
  case '[':
  {
    value->type          = JSON_ARRAY;
    value->arr.arrayCap  = 0;
    value->arr.arraySize = 0;
    return parseJsonArray(&value->arr, buffer);
  }
  case 't':
  {
    if (parseKeyword(buffer, "rue", 3))
    {
      value->type = JSON_BOOL;
      value->b    = true;
      buffer->curr += 4;
      return true;
    }
    printf("Got 't' but wasn't true?\n");
    return false;
  }
  case 'f':
  {
    if (parseKeyword(buffer, "alse", 4))
    {
      value->type = JSON_BOOL;
      value->b    = false;
      buffer->curr += 5;
      return true;
    }
    printf("Got 'f' but wasn't false?\n");
    return false;
  }
  case 'n':
  {
    if (parseKeyword(buffer, "ull", 3))
    {
      value->type = JSON_NULL;
      buffer->curr += 4;
      return true;
    }
    printf("Got 'n' but wasn't null?\n");
    return false;
  }
  default:
  {
    printf("Unknown value token '%c'\n", currentChar);
    return false;
  }
  }
}

bool deserializeFromFile(Json* json, const char* filename)
{
  String fileContent;
  bool   result;

  result = ah_ReadFile(&fileContent, filename);
  if (!result)
  {
    printf("Failed to read file\n");
    return false;
  }
  bool   res;
  bool   first = false;

  Buffer buffer;
  buffer.buffer = (u8*)fileContent.buffer;
  buffer.curr   = 0;
  buffer.len    = fileContent.len;

  while (!first)
  {
    switch (getCurrentCharBuffer(&buffer))
    {
    case '{':
    {
      json->headType = JSON_OBJECT;
      initJsonObject(&json->obj);
      res   = parseJsonObject(&json->obj, &buffer);
      first = true;
      break;
    }
    case '[':
    {
      json->headType        = JSON_ARRAY;
      json->array.arrayCap  = 0;
      json->array.arraySize = 0;
      res                   = parseJsonArray(&json->array, &buffer);
      first                 = true;
      break;
    }
    case ' ':
    {
    }
    case '\n':
    {
    }
    case '\t':
    {
      advanceBuffer(&buffer);
      break;
    }
    default:
    {
      printf("Default: %c\n", getCurrentCharBuffer(&buffer));
      json->headType = JSON_VALUE;
      res            = parseJsonValue(&json->value, &buffer);
      first          = true;
      break;
    }
    }
  }
  if (!res)
  {
    printf("Failed to parse something\n");
    return false;
  }
  if (buffer.curr != fileContent.len)
  {
    printf("Didn't reach eof after parsing first value? %ld %ld\n", buffer.curr, fileContent.len);
    return false;
  }
  return true;
}
