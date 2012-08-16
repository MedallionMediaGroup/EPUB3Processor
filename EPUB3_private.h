#ifndef EPUB3_private_h
#define EPUB3_private_h

#include "unzip.h"

#define EXPORT __attribute__((visibility("default")))

const char * kEPUB3TypeID;
const char * kEPUB3MetadataTypeID;

#pragma mark - Type definitions

typedef struct EPUB3Type {
  const char *typeID;
  uint32_t refCount;
} EPUB3Type;

struct EPUB3Object {
  EPUB3Type _type;
};
typedef struct EPUB3Object *EPUB3ObjectRef;

struct EPUB3 {
  EPUB3Type _type;
  EPUB3MetadataRef metadata;
  char * archivePath;
  unzFile archive;
  unsigned long archiveFileCount;
};

struct EPUB3Metadata {
  EPUB3Type _type;
  char *title;
};

#pragma mark - Function Declarations

static void _EPUB3ObjectRelease(void *object);
static void * _EPUB3ObjectRetain(void *object);
static void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID);
unsigned long numberOfFilesInZip(unzFile file);
void copyStringInto(char **dest, const char *src);


#endif
