#include "EPUB3.h"

#define EXPORT __attribute__((visibility("default")))

#pragma mark - Type definitions

static const char * kEPUB3TypeID = "_EPUB3_t";
static const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";

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
};

struct EPUB3Metadata {
  EPUB3Type _type;
  const char *title;
};

#pragma mark - Function Declarations
#pragma mark Private
static void _EPUB3ObjectRelease(void *object);
static void * _EPUB3ObjectRetain(void *object);
static void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID);

#pragma mark Public
EXPORT EPUB3Ref EPUB3Retain(EPUB3Ref epub);
EXPORT void EPUB3Release(EPUB3Ref epub);
EXPORT EPUB3MetadataRef EPUB3MetadataRetain(EPUB3MetadataRef metadata);
EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata);

EXPORT EPUB3Ref EPUB3Create();

EXPORT EPUB3MetadataRef EPUB3MetadataCreate();
EXPORT EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub);
EXPORT void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title);
EXPORT void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata);

#pragma mark - Memory Management (Reference Counting)

static void _EPUB3ObjectRelease(void *object) {
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount -= 1;
  if(obj->_type.refCount == 0) {
    free(obj);
  }
}

static void * _EPUB3ObjectRetain(void *object) {
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount += 1;
  return obj;
}

static void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID) {
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.typeID = typeID;
  obj->_type.refCount = 1;
  return obj;
}

EXPORT EPUB3Ref EPUB3Retain(EPUB3Ref epub) {
  (void)EPUB3MetadataRetain(epub->metadata);
  return _EPUB3ObjectRetain(epub);
}

EXPORT void EPUB3Release(EPUB3Ref epub) {
  EPUB3MetadataRelease(epub->metadata);
  _EPUB3ObjectRelease(epub);
}

EXPORT EPUB3MetadataRef EPUB3MetadataRetain(EPUB3MetadataRef metadata) {
  return _EPUB3ObjectRetain(metadata);
}

EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata) {
  _EPUB3ObjectRelease(metadata);
}

#pragma mark - EPUB3Ref

EXPORT EPUB3Ref EPUB3Create() {
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  return memory;
}

#pragma mark - EPUB3MetadataRef

EXPORT EPUB3MetadataRef EPUB3MetadataCreate() {
  EPUB3MetadataRef memory = malloc(sizeof(struct EPUB3Metadata));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3MetadataTypeID);
  memory->title = NULL;
  return memory;
}

EXPORT EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub) {
  EPUB3MetadataRef copy = EPUB3MetadataCreate();
  EPUB3MetadataSetTitle(copy, epub->metadata->title);
  return copy;
}

EXPORT void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title) {
  char * titleCopy = malloc(sizeof(char) * (strlen(title) + 1));
  (void)strcpy(titleCopy, title);
  metadata->title = titleCopy;
}

EXPORT void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata) {
  if(epub->metadata != NULL) {
    EPUB3MetadataRelease(epub->metadata);
  }
  epub->metadata = EPUB3MetadataRetain(metadata);
}

