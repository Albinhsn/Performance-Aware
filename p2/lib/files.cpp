#include "files.h"
#include "common.h"

void saveTarga(struct Image* image, const char* filename)
{
  printf("Saving at '%s'\n", filename);
  FILE*              filePtr;
  unsigned long      count;

  struct TargaHeader h;
  memset(&h.header[0], 0, 18);
  h.width          = image->width;
  h.height         = image->height;
  h.imageType      = 2;
  h.imagePixelSize = 32;

  filePtr          = fopen(filename, "wb");
  if (!filePtr)
  {
    printf("Failed to open file\n");
    exit(2);
  }
  count = fwrite(&h, sizeof(struct TargaHeader), 1, filePtr);
  if (count != 1)
  {
    printf("ERROR: Failed to write into header\n");
    return;
  }

  u32 imageSize = 4 * image->width * image->height;

  // Read in the targa image data.
  count = fwrite(image->data, 1, imageSize, filePtr);
  if (count != imageSize)
  {
    printf("ERROR: count after write doesn't equal imageSize %ld %d\n", count, imageSize);
    return;
  }

  if (fclose(filePtr) != 0)
  {
    printf("Failed to close file\n");
    return;
  }
}


bool ah_ReadFile(struct String* string, const char* fileName)
{
  FILE* filePtr;
  long  fileSize, count;
  char* buffer;
  int   error;

  filePtr = fopen(fileName, "r");
  if (!filePtr)
  {
    return false;
  }

  fseek(filePtr, 0, SEEK_END);
  fileSize                 = ftell(filePtr);

  string->len              = fileSize;
  string->buffer           = (u8*)malloc(sizeof(u8) * (fileSize + 1));
  string->buffer[fileSize] = '\0';

  fseek(filePtr, 0, SEEK_SET);
  count = fread(string->buffer, 1, fileSize, filePtr);
  if (count != fileSize)
  {
    free(string->buffer);
    return false;
  }

  error = fclose(filePtr);
  if (error != 0)
  {
    free(string->buffer);
    return false;
  }

  return true;
}

// struct Image *LoadTarga(const char *filename) {

//   struct Image *image = (struct Image *)malloc(sizeof(struct Image));
//   struct TargaHeader targaFileHeader;

//   FILE *filePtr;
//   unsigned long count, imageSize;
//   unsigned char *targaData;
//   unsigned char *targaImage;

//   // Open the targa file for reading in binary.
//   filePtr = fopen(filename, "rb");
//   if (filePtr == NULL) {
//     printf("ERROR: file doesn't exist %s\n", filename);
//     return NULL;
//   }

//   // Read in the file header.
//   count = fread(&targaFileHeader, sizeof(struct TargaHeader), 1, filePtr);
//   if (count != 1) {
//     printf("ERROR: Failed to read into header\n");
//     return NULL;
//   }

//   // Get the important information from the header.
//   image->width = (int)targaFileHeader.width;
//   image->height = (int)targaFileHeader.height;
//   image->bpp = (int)targaFileHeader.bpp;

//   // Calculate the size of the 32 bit image data.
//   if (image->bpp == 32) {
//     imageSize = image->width * image->height * 4;
//   } else if (image->bpp == 24) {
//     imageSize = image->width * image->height * 3;
//   }

//   // Allocate memory for the targa image data.
//   targaImage = (unsigned char *)malloc(sizeof(unsigned char) * imageSize);

//   // Read in the targa image data.
//   count = fread(targaImage, 1, imageSize, filePtr);
//   if (count != imageSize) {
//     printf("ERROR: count read doesn't equal imageSize\n");
//     return NULL;
//   }

//   if (fclose(filePtr) != 0) {
//     return NULL;
//   }

//   targaData = (unsigned char *)malloc(sizeof(unsigned char) * imageSize);
//   bool bit32 = image->bpp == 32;

//   for (int idx = 0; idx < image->height * image->width; idx++) {
//     targaData[idx] = targaImage[idx + 2];     // Red
//     targaData[idx + 1] = targaImage[idx + 1]; // Green
//     targaData[idx + 2] = targaImage[idx];     // Blue
//     if (bit32) {
//       targaData[idx + 3] = targaImage[idx + 3]; // Alpha
//     }
//   }

//   free(targaImage);
//   image->data = targaData;

//   return image;
// }
