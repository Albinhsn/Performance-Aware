#include "json.h"
#include "../haversine.h"
#include "common.h"
#include "files.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CURRENT_CHAR(buffer, curr) (buffer[*curr])
#define ADVANCE(curr)              ((*curr)++)
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

static inline void resizeObject(JsonObject* obj)
{
  if (obj->cap == 0)
  {
    obj->cap    = 4;
    obj->values = (JsonValue**)malloc(sizeof(JsonValue*) * obj->cap);
    obj->keys   = (char**)malloc(sizeof(char*) * obj->cap);
  }
  else if (obj->size >= obj->cap)
  {
    obj->cap *= 2;
    obj->values = realloc(obj->values, obj->cap * sizeof(JsonValue));
    obj->keys   = realloc(obj->keys, obj->cap * sizeof(char*));
  }
  for (i32 i = obj->size; i < obj->cap; i++)
  {
    obj->values[i] = (JsonValue*)malloc(sizeof(JsonValue));
  }
}

static inline void resizeArray(JsonArray* arr)
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
}

static void serializeJsonValue(JsonValue* value, FILE* filePtr);
static void serializeJsonObject(JsonObject* object, FILE* filePtr);

static void serializeJsonArray(JsonArray* arr, FILE* filePtr)
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

static void serializeJsonObject(JsonObject* object, FILE* filePtr)
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

static void serializeJsonValue(JsonValue* value, FILE* filePtr)
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

static bool parseNumber(f64* number, char* buffer, u64* curr)
{
  u64 start = *curr;
  ADVANCE(curr);
  char line[32];
  while (isdigit(CURRENT_CHAR(buffer, curr)))
  {
    ADVANCE(curr);
  }
  if (CURRENT_CHAR(buffer, curr) == '.')
  {
    ADVANCE(curr);
    while (isdigit(CURRENT_CHAR(buffer, curr)))
    {
      ADVANCE(curr);
    }
  }
  u64 size = (*curr) - start;
  strncpy(&line[0], &buffer[start], size);
  line[size] = '\0';
  *number    = convertMeBack(strtoul(line, NULL, 10));

  return true;
}
static bool parseString(char** key, char* buffer, u64* curr)
{
  ADVANCE(curr);
  u64 start = *curr;
  while (buffer[*curr] != '"')
  {
    (*curr)++;
  }
  u64 len     = ((*curr) - start);
  *key        = (char*)malloc(sizeof(char) * (1 + len));
  (*key)[len] = '\0';
  strncpy(*key, &buffer[start], len);
  (*curr)++;
  return true;
}

static u64 skipWhitespace(char* buffer)
{
  u64 res = 0;
  while (buffer[res] == ' ' || buffer[res] == '\n')
  {
    res++;
  }
  return res;
}

static bool consumeToken(char got, char expected, u64* curr)
{
  if (expected != got)
  {
    printf("Expected '%c' but got '%c'\n", expected, got);
    return false;
  }
  (*curr)++;
  return true;
}

bool parseJsonValue(JsonValue* value, char* buffer, u64* curr);
bool parseJsonArray(JsonArray* arr, char* buffer, u64* curr);

bool parseKeyValuePair(JsonObject* obj, char* buffer, u64* curr)
{
  resizeObject(obj);

  bool res = parseString(&obj->keys[obj->size], buffer, curr);
  if (!res)
  {
    return false;
  }
  *curr += skipWhitespace(&buffer[*curr]);

  if (!consumeToken(buffer[*curr], ':', curr))
  {
    return false;
  }

  res = parseJsonValue(obj->values[obj->size], buffer, curr);
  if (!res)
  {
    return false;
  }
  obj->size++;

  *curr += skipWhitespace(&buffer[*curr]);
  return true;
}

bool parseJsonObject(JsonObject* obj, char* buffer, u64* curr)
{
  ADVANCE(curr);
  (*curr) = *curr + skipWhitespace(&buffer[*curr]);
  // end or string
  while (buffer[*curr] != '}')
  {
    bool res = parseKeyValuePair(obj, buffer, curr);
    if (!res)
    {
      return false;
    }

    (*curr) += skipWhitespace(&buffer[*curr]);
    if (buffer[*curr] == ',')
    {
      // It's illegal to have a ',' and then end it
      ADVANCE(curr);
    }
    (*curr) += skipWhitespace(&buffer[*curr]);
  }
  ADVANCE(curr);
  return true;
}

bool parseJsonArray(JsonArray* arr, char* buffer, u64* curr)
{
  (*curr)++;
  *curr += skipWhitespace(&buffer[*curr]);
  bool res;
  while (buffer[*curr] != ']')
  {
    resizeArray(arr);
    res = parseJsonValue(&arr->values[arr->arraySize], buffer, curr);
    if (!res)
    {
      return false;
    }
    arr->arraySize++;
    *curr += skipWhitespace(&buffer[*curr]);
    if (buffer[*curr] == ',')
    {
      ADVANCE(curr);
    }
  }
  ADVANCE(curr);

  return true;
}
bool parseJsonValue(JsonValue* value, char* buffer, u64* curr)
{
  *curr += skipWhitespace(&buffer[*curr]);
  if (isdigit(buffer[*curr]) || buffer[*curr] == '-')
  {
    value->type = JSON_NUMBER;
    return parseNumber(&value->number, buffer, curr);
  }
  switch (buffer[*curr])
  {
  case '\"':
  {
    value->type = JSON_STRING;
    return parseString(&value->string, buffer, curr);
  }
  case '{':
  {
    value->type     = JSON_OBJECT;
    value->obj.cap  = 0;
    value->obj.size = 0;

    return parseJsonObject(&value->obj, buffer, curr);
  }
  case '[':
  {
    value->type          = JSON_ARRAY;
    value->arr.arrayCap  = 0;
    value->arr.arraySize = 0;
    return parseJsonArray(&value->arr, buffer, curr);
  }
  case 't':
  {
    if (buffer[*curr + 1] == 'r' && buffer[*curr + 2] == 'u' && buffer[*curr + 3] == 'e')
    {
      value->type = JSON_BOOL;
      value->b    = true;
      (*curr) += 4;
      return true;
    }
    printf("Got 't' but wasn't true?\n");
    return false;
  }
  case 'f':
  {
    if (buffer[*curr + 1] == 'a' && buffer[*curr + 2] == 'l' && buffer[*curr + 3] == 's' && buffer[*curr + 4] == 'e')
    {
      value->type = JSON_BOOL;
      value->b    = false;
      (*curr) += 5;
      return true;
    }
    printf("Got 'f' but wasn't false?\n");
    return false;
  }
  case 'n':
  {
    if (buffer[*curr + 1] == 'u' && buffer[*curr + 2] == 'l' && buffer[*curr + 3] == 'l')
    {
      value->type = JSON_NULL;
      (*curr) += 4;
      return true;
    }
    printf("Got 'n' but wasn't null?\n");
    return false;
  }
  default:
  {
    printf("Unknown value token '%c'\n", buffer[*curr]);
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
  u64  curr = 0;
  bool res;
  bool first = false;
  while (!first)
  {
    switch (fileContent.buffer[curr])
    {
    case '{':
    {
      json->headType = JSON_OBJECT;
      json->obj.cap  = 0;
      json->obj.size = 0;
      res            = parseJsonObject(&json->obj, fileContent.buffer, &curr);
      first          = true;
      break;
    }
    case '[':
    {
      json->headType        = JSON_ARRAY;
      json->array.arrayCap  = 0;
      json->array.arraySize = 0;
      res                   = parseJsonArray(&json->array, fileContent.buffer, &curr);
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
      curr++;
      break;
    }
    default:
    {
      printf("Default: %c\n", fileContent.buffer[curr]);
      json->headType = JSON_VALUE;
      res            = parseJsonValue(&json->value, fileContent.buffer, &curr);
      first          = true;
      break;
    }
    }
  }
  if (!res)
  {
    printf("Failed to parse json value\n");
    return false;
  }
  if (curr != fileContent.len)
  {
    printf("Didn't reach eof after parsing first value? %ld %ld\n", curr, fileContent.len);
    return false;
  }
  return true;
}
