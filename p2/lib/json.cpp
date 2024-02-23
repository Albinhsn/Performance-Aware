#include "json.h"
#include "../arena.h"
#include "common.h"
#include "files.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>

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
    printf("\"%.*s\"", (i32)value->string.len, value->string.buffer);
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
    printf("\"%.*s\":", (i32)object->keys[i].len, object->keys[i].buffer);
    debugJsonValue(&object->values[i]);
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
    obj->values = (JsonValue*)realloc(obj->values, obj->cap * sizeof(JsonValue));
    obj->keys   = (String*)realloc(obj->keys, obj->cap * sizeof(String));
  }
}

inline void resizeArray(JsonArray* arr)
{
  if (arr->arraySize >= arr->arrayCap)
  {
    arr->arrayCap *= 2;
    arr->values = (JsonValue*)realloc(arr->values, arr->arrayCap * sizeof(JsonValue));
  }
}

void addElementToJsonObject(JsonObject* obj, String key, JsonValue value)
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
void initJsonArray(Arena* arena, JsonArray* array)
{
  array->arraySize = 0;
  array->arrayCap  = 4;
  array->values    = (JsonValue*)malloc(sizeof(JsonValue) * array->arrayCap);
}
void initJsonObject(Arena* arena, JsonObject* obj)
{
  // TimeFunction;
  obj->size   = 0;
  obj->cap    = 4;
  obj->values = ArenaPushArray(arena, JsonValue, obj->cap);
  obj->keys   = ArenaPushArray(arena, String, obj->cap);
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
    fprintf(filePtr, "\"%.*s\":", (i32)object->keys[i].len, object->keys[i].buffer);
    serializeJsonValue(&object->values[i], filePtr);
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
    fprintf(filePtr, "%.*g", DBL_DECIMAL_DIG, (double)value->number);
    break;
  }
  case JSON_ARRAY:
  {
    serializeJsonArray(&value->arr, filePtr);
    break;
  }
  case JSON_STRING:
  {
    fprintf(filePtr, "\"%.*s\"", (i32)value->string.len, value->string.buffer);
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

inline f64 convertJsonNumber(Buffer* buffer)
{
  f64 result = 0.0f;
  while (isdigit(getCurrentCharBuffer(buffer)))
  {
    u8 ch  = getCurrentCharBuffer(buffer) - (u8)'0';
    result = 10.0 * result + (f64)ch;
    advanceBuffer(buffer);
  }
  return result;
}

f64 parseNumber(Buffer* buffer)
{
  // TimeFunction;
  f64 sign = getCurrentCharBuffer(buffer) == '-' ? -1.0f : 1.0f;
  if (sign == -1.0f)
  {
    advanceBuffer(buffer);
  }

  f64 result = convertJsonNumber(buffer);

  if (getCurrentCharBuffer(buffer) == '.')
  {
    advanceBuffer(buffer);
    f64 c = 1.0 / 10.0;
    while (isdigit(getCurrentCharBuffer(buffer)))
    {
      u8 ch = getCurrentCharBuffer(buffer) - (u8)'0';
      result += c * (f64)ch;
      c *= 1.0 / 10.0;
      advanceBuffer(buffer);
    }
  }

  char curr = getCurrentCharBuffer(buffer);
  if (curr == 'e' || curr == 'E')
  {
    advanceBuffer(buffer);
    f64 exponentSign = getCurrentCharBuffer(buffer) == '-' ? -1.0f : 1.0f;
    advanceBuffer(buffer);
    f64 exponent = convertJsonNumber(buffer) * exponentSign;
    result *= pow(10.0, exponent);
  }

  return sign * result;
}

bool parseString(String* key, Buffer* buffer)
{
  // TimeFunction;
  advanceBuffer(buffer);
  u64 start = buffer->curr;
  while (getCurrentCharBuffer(buffer) != '"')
  {
    advanceBuffer(buffer);
  }
  u64 len     = buffer->curr - start;
  key->len    = len;
  key->buffer = &buffer->buffer[start];

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

bool parseJsonValue(Arena* arena, JsonValue* value, Buffer* buffer);
bool parseJsonArray(Arena* arena, JsonArray* arr, Buffer* buffer);

bool parseKeyValuePair(Arena* arena, JsonObject* obj, Buffer* buffer)
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

  res = parseJsonValue(arena, &obj->values[obj->size], buffer);
  if (!res)
  {
    return false;
  }
  obj->size++;

  skipWhitespace(buffer);
  return true;
}

bool parseJsonObject(Arena* arena, JsonObject* obj, Buffer* buffer)
{
  advanceBuffer(buffer);
  skipWhitespace(buffer);

  // end or string
  while (getCurrentCharBuffer(buffer) != '}')
  {
    bool res = parseKeyValuePair(arena, obj, buffer);
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

bool parseJsonArray(Arena* arena, JsonArray* arr, Buffer* buffer)
{
  TimeBlock("JsonArray");

  advanceBuffer(buffer);
  skipWhitespace(buffer);
  bool res;
  while (getCurrentCharBuffer(buffer) != ']')
  {
    resizeArray(arr);
    res = parseJsonValue(arena, &arr->values[arr->arraySize], buffer);
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
bool parseKeyword(Buffer* buffer, const char* expected, u8 len)
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

bool parseJsonValue(Arena* arena, JsonValue* value, Buffer* buffer)
{
  // TimeFunction;
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
    initJsonObject(arena, &value->obj);
    return parseJsonObject(arena, &value->obj, buffer);
  }
  case '[':
  {
    value->type = JSON_ARRAY;
    initJsonArray(arena, &value->arr);
    return parseJsonArray(arena, &value->arr, buffer);
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

bool deserializeFromString(Json* json, Arena* arena, String fileContent)
{
  // TimeFunction;
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
      initJsonObject(arena, &json->obj);
      res   = parseJsonObject(arena, &json->obj, &buffer);
      first = true;
      break;
    }
    case '[':
    {
      json->headType = JSON_ARRAY;
      initJsonArray(arena, &json->array);
      res   = parseJsonArray(arena, &json->array, &buffer);
      first = true;
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
      res            = parseJsonValue(arena, &json->value, &buffer);
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

JsonValue* lookupJsonElement(JsonObject* obj, const char* lookupKey)
{
  u32 keyLength = strlen(lookupKey);
  for (i32 i = 0; i < obj->size; i++)
  {
    String key    = obj->keys[i];
    u32    minLen = keyLength < key.len ? keyLength : key.len;
    if (strncmp(lookupKey, (char*)key.buffer, minLen) == 0)
    {
      return &obj->values[i];
    }
  }
  return NULL;
}
void freeJsonObject(JsonObject* obj)
{
  for (i32 i = 0; i < obj->size; i++)
  {
    freeJsonValue(&obj->values[i]);
  }

  free(obj->values);
}
void freeJsonValue(JsonValue* value)
{
  switch (value->type)
  {
  case JSON_OBJECT:
  {
    freeJsonObject(&value->obj);
    break;
  }
  case JSON_ARRAY:
  {
    freeJsonArray(&value->arr);
    break;
  }
  default:
  {
    break;
  }
  }
}
void freeJsonArray(JsonArray* array)
{
  for (i32 i = 0; i < array->arraySize; i++)
  {
    freeJsonValue(&array->values[i]);
  }
  // free(array->values);
}
void freeJson(Json* json)
{
  // TimeFunction;

  switch (json->headType)
  {
  case JSON_OBJECT:
  {
    freeJsonObject(&json->obj);
    break;
  }
  case JSON_ARRAY:
  {
    freeJsonArray(&json->array);
    break;
  }
  case JSON_VALUE:
  {
    freeJsonValue(&json->value);
    break;
  }
  default:
  {
    break;
  }
  }
}
