#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";

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
  if(epub->archive != NULL) {
    unzClose(epub->archive);
    epub->archive = NULL;
  }
  free(epub->archivePath);
  _EPUB3ObjectRelease(epub);
}

EXPORT EPUB3MetadataRef EPUB3MetadataRetain(EPUB3MetadataRef metadata) {
  return _EPUB3ObjectRetain(metadata);
}

EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata) {
  free(metadata->title);
  _EPUB3ObjectRelease(metadata);
}

#pragma mark - EPUB3Ref

EXPORT EPUB3Ref EPUB3Create() {
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  return memory;
}

EXPORT EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path) {
  EPUB3Ref epub = EPUB3Create();
  unzFile archive = unzOpen(path);
  epub->archive = archive;
  epub->archiveFileCount = numberOfFilesInZip(archive);
  //TODO: parse and load metadata?
  return epub;
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
  if(metadata->title != NULL) {
    free(metadata->title);
  }
  if(title == NULL) {
    metadata->title = NULL;
    return;
  }
  char * titleCopy = strdup(title);
  metadata->title = titleCopy;
}

EXPORT void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata) {
  if(epub->metadata != NULL) {
    EPUB3MetadataRelease(epub->metadata);
  }
  epub->metadata = EPUB3MetadataRetain(metadata);
}

unsigned long numberOfFilesInZip(unzFile file)
{
  unz_global_info gi;
	int err = unzGetGlobalInfo(file, &gi);
	if (err != UNZ_OK)
    return err;
	
	return gi.number_entry;
}
