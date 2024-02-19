#include "string.h"

void ah_strcpyString(struct String *s1, struct String *s2) {
  s1->len = s2->len;
  s1->buffer = (char *)malloc(sizeof(char) * s1->len);
  memcpy(s1->buffer, s2->buffer, s2->len);
}

char *ah_strcpy(char *buffer, struct String *s2) {
  memcpy(buffer, s2->buffer, s2->len);
  return buffer;
}
