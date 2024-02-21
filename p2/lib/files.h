#ifndef FILES_H
#define FILES_H
#include "string.h"
#include <stdbool.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef float f32;
typedef double f64;

typedef int8_t i8;
typedef int16_t i16;
typedef int i32;
typedef int64_t i64;

struct Image {
  int width, height;
  int bpp;
  unsigned char *data;
};
struct TargaHeader {
  union {
    u8 header[18];
    struct {
      u8 charactersInIdentificationField;
      u8 colorMapType;
      u8 imageType;
      u8 colorMapSpec[5];
      u16 xOrigin;
      u16 yOrigin;
      u16 width;
      u16 height;
      u8 imagePixelSize;
      u8 imageDescriptor;
    };
  };
};

struct Image *LoadTarga(const char *filename);
bool ah_ReadFile(struct String *string, const char *fileName);

char *ah_strcpy(char *buffer, struct String *s2);

#endif
