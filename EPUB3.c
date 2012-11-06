#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";
const char * kEPUB3ManifestTypeID = "_EPUB3Manifest_t";
const char * kEPUB3ManifestItemTypeID = "_EPUB3ManifestItem_t";
const char * kEPUB3SpineTypeID = "_EPUB3Spine_t";
const char * kEPUB3SpineItemTypeID = "_EPUB3SpineItem_t";
const char * kEPUB3TocTypeID = "_EPUB3Toc_t";
const char * kEPUB3TocItemTypeID = "_EPUB3TocItem_t";


#ifndef PARSE_CONTEXT_STACK_DEPTH
#define PARSE_CONTEXT_STACK_DEPTH 64
#endif

#pragma mark - Public Query API

EXPORT int32_t EPUB3CountOfSequentialResources(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->spine != NULL);
  return epub->spine->linearItemCount;
}

EXPORT EPUB3Error EPUB3GetPathsOfSequentialResources(EPUB3Ref epub, const char ** resources)
{
  assert(epub != NULL);
  assert(epub->spine != NULL);

  EPUB3Error error = kEPUB3Success;

  if(epub->spine->linearItemCount > 0) {
    int32_t count = 0;
    EPUB3SpineItemListItemPtr itemPtr = epub->spine->head;
    while(itemPtr != NULL) {
      if(itemPtr->item->isLinear) {
        resources[count] = itemPtr->item->manifestItem->href;
        count++;
      }
      itemPtr = itemPtr->next;
    }

  }

  return error;
}

#pragma mark - Base Object

void EPUB3ObjectRelease(void *object)
{
  if(object == NULL) return;

  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount--;
  if(obj->_type.refCount == 0) {
    EPUB3_FREE_AND_NULL(obj);
  }
}

void EPUB3ObjectRetain(void *object)
{
  if(object == NULL) return;

  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount++;
}

void * EPUB3ObjectInitWithTypeID(void *object, const char *typeID)
{
  assert(object != NULL);
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.typeID = typeID;
  obj->_type.refCount = 1;
  return obj;
}

#pragma mark - Main EPUB3 Object

EPUB3Ref EPUB3Create()
{
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  memory->manifest = NULL;
  memory->spine = NULL;
  memory->archive = NULL;
  memory->archivePath = NULL;
  memory->archiveFileCount = 0;
  return memory;
}

EXPORT EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path, EPUB3Error *error)
{
  assert(path != NULL);

  EPUB3Ref epub = EPUB3Create();
  *error = EPUB3PrepareArchiveAtPath(epub, path);
  if(*error != kEPUB3Success) {
    EPUB3Release(epub);
    return NULL;
  }

  *error = EPUB3InitAndValidate(epub);
  if(*error != kEPUB3Success) {
    EPUB3Release(epub);
    return NULL;
  }

  return epub;
}

EPUB3Error EPUB3PrepareArchiveAtPath(EPUB3Ref epub, const char * path)
{
  assert(epub != NULL);
  assert(path != NULL);

  EPUB3Error error = kEPUB3Success;
  unzFile archive = unzOpen(path);
  epub->archive = archive;
  epub->archiveFileCount = EPUB3GetFileCountInArchive(archive);
  epub->archivePath = strdup(path);
  return error;
}

EPUB3Error EPUB3InitAndValidate(EPUB3Ref epub)
{
  assert(epub != NULL);
  char * opfPath = NULL;
  EPUB3Error error = EPUB3CopyRootFilePathFromContainer(epub, &opfPath);
  if(error != kEPUB3Success) {
    fprintf(stderr, "Error (%d[%d]) opening and validating epub file at %s.\n", error, __LINE__, epub->archivePath);
  }
  error = EPUB3InitFromOPF(epub, opfPath);
  if(error != kEPUB3Success) {
    fprintf(stderr, "Error (%d[%d]) parsing epub file at %s.\n", error, __LINE__, epub->archivePath);
  }
  EPUB3_FREE_AND_NULL(opfPath);
  return error;
}

EXPORT void EPUB3Retain(EPUB3Ref epub)
{
  if(epub == NULL) return;

  EPUB3MetadataRetain(epub->metadata);
  EPUB3ManifestRetain(epub->manifest);
  EPUB3SpineRetain(epub->spine);
  EPUB3ObjectRetain(epub);
}

EXPORT void EPUB3Release(EPUB3Ref epub)
{
  if(epub == NULL) return;

  if(epub->_type.refCount == 1) {
    if(epub->archive != NULL) {
      unzClose(epub->archive);
      epub->archive = NULL;
    }
    EPUB3_FREE_AND_NULL(epub->archivePath);
  }

  EPUB3MetadataRelease(epub->metadata);
  EPUB3ManifestRelease(epub->manifest);
  EPUB3SpineRelease(epub->spine);
  EPUB3ObjectRelease(epub);
}

EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub)
{
  assert(epub != NULL);

  if(epub->metadata == NULL) {
    return NULL;
  }
  EPUB3MetadataRef copy = EPUB3MetadataCreate();
  copy->ncxItem = epub->metadata->ncxItem;
  EPUB3ManifestItemRetain(copy->ncxItem);
  (void)EPUB3MetadataSetTitle(copy, epub->metadata->title);
  (void)EPUB3MetadataSetIdentifier(copy, epub->metadata->identifier);
  (void)EPUB3MetadataSetLanguage(copy, epub->metadata->language);
  return copy;
}

void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata)
{
  assert(epub != NULL);

  if(epub->metadata != NULL) {
    EPUB3MetadataRelease(epub->metadata);
  }
  if(metadata != NULL) {
    EPUB3MetadataRetain(metadata);
  }
  epub->metadata = metadata;
}

void EPUB3SetManifest(EPUB3Ref epub, EPUB3ManifestRef manifest)
{
  assert(epub != NULL);

  if(epub->manifest != NULL) {
    EPUB3ManifestRelease(epub->manifest);
  }
  if(manifest != NULL) {
    EPUB3ManifestRetain(manifest);
  }
  epub->manifest = manifest;
}

void EPUB3SetSpine(EPUB3Ref epub, EPUB3SpineRef spine)
{
  assert(epub != NULL);

  if(epub->spine != NULL) {
    EPUB3SpineRelease(epub->spine);
  }
  if(spine != NULL) {
    EPUB3SpineRetain(spine);
  }
  epub->spine = spine;
}

void EPUB3SetStringValue(char ** location, const char *value)
{
  EPUB3_FREE_AND_NULL(*location);
  if(value == NULL) {
    return;
  }
  char * valueCopy = strdup(value);
  *location = valueCopy;
}

char * EPUB3CopyStringValue(char ** location)
{
  if(*location == NULL) return NULL;

  char * copy = strdup(*location);
  return copy;
}

EXPORT char * EPUB3CopyTitle(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->title));
}

EXPORT char * EPUB3CopyIdentifier(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->identifier));
}

EXPORT char * EPUB3CopyLanguage(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->language));
}

EXPORT char * EPUB3CopyCoverImagePath(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  assert(epub->manifest != NULL);

  if(epub->metadata->coverImageId == NULL) return NULL;

  EPUB3ManifestItemListItemPtr coverItemPtr = EPUB3ManifestFindItemWithId(epub->manifest, epub->metadata->coverImageId);
  return EPUB3CopyStringValue(&(coverItemPtr->item->href));
}

EXPORT EPUB3Error EPUB3CopyCoverImage(EPUB3Ref epub, void ** bytes, uint32_t * byteCount)
{
  assert(epub != NULL);

  *byteCount = 0U;
  char * path = EPUB3CopyCoverImagePath(epub);
  if(path == NULL)
  {
      return kEPUB3FileNotFoundInArchiveError;
  }
  char * rootFilePath = NULL;
  EPUB3Error error = EPUB3CopyRootFilePathFromContainer(epub, &rootFilePath);
  if(error == kEPUB3Success) {
    char * rootPath = EPUB3CopyOfPathByDeletingLastPathComponent(rootFilePath);
    char * fullPath = EPUB3CopyOfPathByAppendingPathComponent(rootPath, path);
    error = EPUB3CopyFileIntoBuffer(epub, bytes, NULL, byteCount, fullPath);
    EPUB3_FREE_AND_NULL(rootPath);
    EPUB3_FREE_AND_NULL(fullPath);
  }
  EPUB3_FREE_AND_NULL(path);
  EPUB3_FREE_AND_NULL(rootFilePath);
  return error;
}

#pragma mark - Table of Contents

EPUB3TocRef EPUB3TocCreate()
{
  EPUB3TocRef memory = malloc(sizeof(struct EPUB3Toc));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3TocTypeID);
  memory->itemCount = 0;
  memory->head = NULL;
  memory->tail = NULL;
  return memory;
}

void EPUB3TocRetain(EPUB3TocRef toc)
{
  if(toc == NULL) return;

  EPUB3TocItemListItemPtr itemPtr;
  for(itemPtr = toc->head; itemPtr != NULL; itemPtr = itemPtr->next) {
    EPUB3TocItemRetain(itemPtr->item);
  }
  EPUB3ObjectRetain(toc);
}

void EPUB3TocRelease(EPUB3TocRef toc)
{
  if(toc == NULL) return;
  if(toc->_type.refCount == 1) {
    EPUB3TocItemListItemPtr itemPtr = toc->head;
    int totalItemsToFree = toc->itemCount;
    while(itemPtr != NULL) {
      assert(--totalItemsToFree >= 0);
      EPUB3TocItemRelease(itemPtr->item);
      EPUB3TocItemListItemPtr tmp = itemPtr;
      itemPtr = itemPtr->next;
      toc->head = itemPtr;
      EPUB3_FREE_AND_NULL(tmp);
    }
    toc->itemCount = 0;
  }
  EPUB3ObjectRelease(toc);
}

EPUB3TocItemRef EPUB3TocItemCreate()
{
  EPUB3TocItemRef memory = malloc(sizeof(struct EPUB3TocItem));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3TocItemTypeID);
  memory->idref = NULL;
  memory->manifestItem = NULL;
  return memory;
}

void EPUB3TocItemRetain(EPUB3TocItemRef item)
{
  if(item == NULL) return;
  EPUB3ObjectRetain(item);
}

void EPUB3TocItemRelease(EPUB3TocItemRef item)
{
  if(item == NULL) return;

  if(item->_type.refCount == 1) {
    item->manifestItem = NULL; // zero weak ref
    EPUB3_FREE_AND_NULL(item->idref);
  }

  EPUB3ObjectRelease(item);
}

void EPUB3TocItemSetManifestItem(EPUB3TocItemRef tocItem, EPUB3ManifestItemRef manifestItem)
{
  assert(tocItem != NULL);
  tocItem->manifestItem = manifestItem;
  tocItem->idref = strdup(manifestItem->itemId);
}

void EPUB3TocAppendItem(EPUB3TocRef toc, EPUB3TocItemRef item)
{
  assert(toc != NULL);
  assert(item != NULL);

  EPUB3TocItemRetain(item);
  EPUB3TocItemListItemPtr itemPtr = (EPUB3TocItemListItemPtr) calloc(1, sizeof(struct EPUB3TocItemListItem));
  itemPtr->item = item;

  if(toc->head == NULL) {
    // First item
    toc->head = itemPtr;
    toc->tail = itemPtr;
  } else {
    toc->tail->next = itemPtr;
    toc->tail = itemPtr;
  }
  toc->itemCount++;
}

#pragma mark - Metadata

void EPUB3MetadataRetain(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;

  EPUB3ManifestItemRetain(metadata->ncxItem);
  EPUB3ObjectRetain(metadata);
}

void EPUB3MetadataRelease(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;

  if(metadata->_type.refCount == 1) {
    EPUB3ManifestItemRelease(metadata->ncxItem);
    metadata->ncxItem = NULL;
    EPUB3_FREE_AND_NULL(metadata->title);
    EPUB3_FREE_AND_NULL(metadata->_uniqueIdentifierID);
    EPUB3_FREE_AND_NULL(metadata->identifier);
    EPUB3_FREE_AND_NULL(metadata->language);
    EPUB3_FREE_AND_NULL(metadata->coverImageId);
  }
  EPUB3ObjectRelease(metadata);
}

EPUB3MetadataRef EPUB3MetadataCreate()
{
  EPUB3MetadataRef memory = malloc(sizeof(struct EPUB3Metadata));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3MetadataTypeID);
  memory->ncxItem = NULL;
  memory->title = NULL;
  memory->_uniqueIdentifierID = NULL;
  memory->identifier = NULL;
  memory->language = NULL;
  memory->coverImageId = NULL;
  return memory;
}

void EPUB3MetadataSetNCXItem(EPUB3MetadataRef metadata, EPUB3ManifestItemRef ncxItem)
{
  assert(metadata != NULL);

  if(metadata->ncxItem != NULL) {
    EPUB3ManifestItemRelease(metadata->ncxItem);
  }
  EPUB3ManifestItemRetain(ncxItem);
  metadata->ncxItem = ncxItem;
}

void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title)
{
  assert(metadata != NULL);
  EPUB3SetStringValue(&(metadata->title), title);
}

void EPUB3MetadataSetIdentifier(EPUB3MetadataRef metadata, const char * identifier)
{
  assert(metadata != NULL);
  EPUB3SetStringValue(&(metadata->identifier), identifier);
}

void EPUB3MetadataSetLanguage(EPUB3MetadataRef metadata, const char * language)
{
  assert(metadata != NULL);
  EPUB3SetStringValue(&(metadata->language), language);
}

void EPUB3MetadataSetCoverImageId(EPUB3MetadataRef metadata, const char * coverImgId)
{
  assert(metadata != NULL);
  EPUB3SetStringValue(&(metadata->coverImageId), coverImgId);
}

#pragma mark - Manifest

void EPUB3ManifestRetain(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;

  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    EPUB3ManifestItemListItemPtr itemPtr = manifest->itemTable[i];
    while(itemPtr != NULL) {
      EPUB3ManifestItemRetain(itemPtr->item);
      itemPtr = itemPtr->next;
    }
  }
  EPUB3ObjectRetain(manifest);
}

void EPUB3ManifestRelease(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {

    EPUB3ManifestItemListItemPtr next = manifest->itemTable[i];
    while(next != NULL) {
      EPUB3ManifestItemRelease(next->item);
      EPUB3ManifestItemListItemPtr tmp = next;
      next = tmp->next;
      if(manifest->_type.refCount == 1) {
        EPUB3_FREE_AND_NULL(tmp);
      }
    }
    if(manifest->_type.refCount == 1) {
      manifest->itemTable[i] = NULL;
    }
  }
  if(manifest->_type.refCount == 1) {
    manifest->itemCount = 0;
  }
  EPUB3ObjectRelease(manifest);
}

void EPUB3ManifestItemRetain(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;

  EPUB3ObjectRetain(item);
}

void EPUB3ManifestItemRelease(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;

  if(item->_type.refCount == 1) {
    EPUB3_FREE_AND_NULL(item->itemId);
    EPUB3_FREE_AND_NULL(item->href);
    EPUB3_FREE_AND_NULL(item->mediaType);
    EPUB3_FREE_AND_NULL(item->properties);
  }

  EPUB3ObjectRelease(item);
}

EPUB3ManifestRef EPUB3ManifestCreate()
{
  EPUB3ManifestRef memory = malloc(sizeof(struct EPUB3Manifest));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestTypeID);
  memory->itemCount = 0;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    memory->itemTable[i] = NULL;
  }
  return memory;
}

EPUB3ManifestItemRef EPUB3ManifestItemCreate()
{
  EPUB3ManifestItemRef memory = malloc(sizeof(struct EPUB3ManifestItem));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestItemTypeID);
  memory->itemId = NULL;
  memory->href = NULL;
  memory->mediaType = NULL;
  memory->properties = NULL;
  return memory;
}

void EPUB3ManifestInsertItem(EPUB3ManifestRef manifest, EPUB3ManifestItemRef item)
{
  assert(manifest != NULL);
  assert(item != NULL);
  assert(item->itemId != NULL);

  EPUB3ManifestItemRetain(item);
  EPUB3ManifestItemListItemPtr itemPtr = EPUB3ManifestFindItemWithId(manifest, item->itemId);
  if(itemPtr == NULL) {
    itemPtr = (EPUB3ManifestItemListItemPtr) malloc(sizeof(struct EPUB3ManifestItemListItem));
    int32_t bucket = SuperFastHash(item->itemId, (int32_t)strlen(item->itemId)) % MANIFEST_HASH_SIZE;
    itemPtr->item = item;
    itemPtr->next = manifest->itemTable[bucket];
    manifest->itemTable[bucket] = itemPtr;
    manifest->itemCount++;
  } else {
    EPUB3ManifestItemRelease(itemPtr->item);
    itemPtr->item = item;
  }
}

EPUB3ManifestItemRef EPUB3ManifestCopyItemWithId(EPUB3ManifestRef manifest, const char * itemId)
{
  assert(manifest != NULL);
  assert(itemId != NULL);

  EPUB3ManifestItemListItemPtr itemPtr = EPUB3ManifestFindItemWithId(manifest, itemId);

  if(itemPtr == NULL) {
    return NULL;
  }

  EPUB3ManifestItemRef item = itemPtr->item;
  EPUB3ManifestItemRef copy = EPUB3ManifestItemCreate();
  copy->itemId = item->itemId != NULL ? strdup(item->itemId) : NULL;
  copy->href = item->href != NULL ? strdup(item->href) : NULL;
  copy->mediaType = item->mediaType != NULL ? strdup(item->mediaType) : NULL;
  copy->properties = item->properties != NULL ? strdup(item->properties) : NULL;
  return copy;
}

EPUB3ManifestItemListItemPtr EPUB3ManifestFindItemWithId(EPUB3ManifestRef manifest, const char * itemId)
{
  assert(manifest != NULL);
  assert(itemId != NULL);

  int32_t bucket = SuperFastHash(itemId, (int32_t)strlen(itemId)) % MANIFEST_HASH_SIZE;
  EPUB3ManifestItemListItemPtr itemPtr = manifest->itemTable[bucket];
  while(itemPtr != NULL) {
    if(strcmp(itemId, itemPtr->item->itemId) == 0) {
      return itemPtr;
    }
    itemPtr = itemPtr->next;
  }
  return NULL;
}

#pragma mark - Spine

EPUB3SpineRef EPUB3SpineCreate()
{
  EPUB3SpineRef memory = malloc(sizeof(struct EPUB3Spine));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3SpineTypeID);
  memory->itemCount = 0;
  memory->linearItemCount = 0;
  memory->head = NULL;
  memory->tail = NULL;
  return memory;
}

void EPUB3SpineRetain(EPUB3SpineRef spine)
{
  if(spine == NULL) return;

  EPUB3SpineItemListItemPtr itemPtr;
  for(itemPtr = spine->head; itemPtr != NULL; itemPtr = itemPtr->next) {
    EPUB3SpineItemRetain(itemPtr->item);
  }
  EPUB3ObjectRetain(spine);
}

void EPUB3SpineRelease(EPUB3SpineRef spine)
{
  if(spine == NULL) return;
  if(spine->_type.refCount == 1) {
    EPUB3SpineItemListItemPtr itemPtr = spine->head;
    int totalItemsToFree = spine->itemCount;
    while(itemPtr != NULL) {
      assert(--totalItemsToFree >= 0);
      EPUB3SpineItemRelease(itemPtr->item);
      EPUB3SpineItemListItemPtr tmp = itemPtr;
      itemPtr = itemPtr->next;
      spine->head = itemPtr;
      EPUB3_FREE_AND_NULL(tmp);
    }
    spine->itemCount = 0;
    spine->linearItemCount = 0;
  }
  EPUB3ObjectRelease(spine);
}

EPUB3SpineItemRef EPUB3SpineItemCreate()
{
  EPUB3SpineItemRef memory = malloc(sizeof(struct EPUB3SpineItem));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3SpineItemTypeID);
  memory->isLinear = kEPUB3_NO;
  memory->idref = NULL;
  memory->manifestItem = NULL;
  return memory;
}

void EPUB3SpineItemRetain(EPUB3SpineItemRef item)
{
  if(item == NULL) return;
  EPUB3ObjectRetain(item);
}

void EPUB3SpineItemRelease(EPUB3SpineItemRef item)
{
  if(item == NULL) return;

  if(item->_type.refCount == 1) {
    item->manifestItem = NULL; // zero weak ref
    EPUB3_FREE_AND_NULL(item->idref);
  }

  EPUB3ObjectRelease(item);
}

void EPUB3SpineItemSetManifestItem(EPUB3SpineItemRef spineItem, EPUB3ManifestItemRef manifestItem)
{
  assert(spineItem != NULL);
  spineItem->manifestItem = manifestItem;
  spineItem->idref = strdup(manifestItem->itemId);
}

void EPUB3SpineAppendItem(EPUB3SpineRef spine, EPUB3SpineItemRef item)
{
  assert(spine != NULL);
  assert(item != NULL);

  EPUB3SpineItemRetain(item);
  EPUB3SpineItemListItemPtr itemPtr = (EPUB3SpineItemListItemPtr) calloc(1, sizeof(struct EPUB3SpineItemListItem));
  itemPtr->item = item;

  if(spine->head == NULL) {
    // First item
    spine->head = itemPtr;
    spine->tail = itemPtr;
  } else {
    spine->tail->next = itemPtr;
    spine->tail = itemPtr;
  }
  spine->itemCount++;
}

#pragma mark - OPF XML Parsing

EPUB3Error EPUB3InitFromOPF(EPUB3Ref epub, const char * opfFilename)
{
  assert(epub != NULL);
  assert(opfFilename != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  if(epub->metadata == NULL) {
    epub->metadata = EPUB3MetadataCreate();
  }

  if(epub->manifest == NULL) {
    epub->manifest = EPUB3ManifestCreate();
  }

  if(epub->spine == NULL) {
    epub->spine = EPUB3SpineCreate();
  }

  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;

  EPUB3Error error = kEPUB3Success;

  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, opfFilename);
  if(error == kEPUB3Success) {
    error = EPUB3ParseOPFFromData(epub, buffer, bufferSize);
    EPUB3_FREE_AND_NULL(buffer);
  }
  if(error == kEPUB3Success && epub->metadata->version == kEPUB3Version_2) {
    // Parse NCX only if this is a v2 epub (per the EPUB 3 spec)
    if(epub->metadata->ncxItem != NULL) {
      char * ncxPath = strdup(epub->metadata->ncxItem->href);
      if(*ncxPath != '/') {
        char * opfRoot = EPUB3CopyOfPathByDeletingLastPathComponent(opfFilename);
        char * fullPath = EPUB3CopyOfPathByAppendingPathComponent(opfRoot, ncxPath);
        free(ncxPath);
        free(opfRoot);
        ncxPath = fullPath;
      }
      error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, ncxPath);
      if(error == kEPUB3Success) {
        error = EPUB3ParseNCXFromData(epub, buffer, bufferSize);
      }
      free(ncxPath);
      EPUB3_FREE_AND_NULL(buffer);
    }
  }
  return error;
}

void EPUB3SaveParseContext(EPUB3XMLParseContextPtr *ctxPtr, EPUB3XMLParseState state, const xmlChar * tagName, int32_t attrCount, char ** attrs, EPUB3Bool shouldParseTextNode)
{
  (*ctxPtr)++;
  (*ctxPtr)->state = state;
  (*ctxPtr)->tagName = tagName;
  (*ctxPtr)->attributeCount = attrCount;
  (*ctxPtr)->attributes = attrs;
  (*ctxPtr)->shouldParseTextNode = shouldParseTextNode;
}

void EPUB3PopAndFreeParseContext(EPUB3XMLParseContextPtr *contextPtr)
{
  EPUB3XMLParseContextPtr ctx = (*contextPtr);
  (*contextPtr)--;
  for (int i = 0; i < ctx->attributeCount; i++) {
    char * key = ctx->attributes[i * 2];
    char * val = ctx->attributes[i * 2 + 1];
    EPUB3_FREE_AND_NULL(key);
    EPUB3_FREE_AND_NULL(val);
  }
}

EPUB3Error EPUB3ProcessXMLReaderNodeForMetadataInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);

  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);

        // Only parse text node for the identifier marked as unique-identifier in the package tag
        // see: http://idpf.org/epub/30/spec/epub30-publications.html#sec-opf-dcidentifier
        if(xmlStrcmp(name, BAD_CAST "identifier") == 0) {
          if(xmlTextReaderHasAttributes(reader)) {
            xmlChar * itemId = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
            if(itemId == NULL) {
              (*context)->shouldParseTextNode = kEPUB3_NO;
            }
            else if(itemId != NULL && xmlStrcmp(itemId, BAD_CAST epub->metadata->_uniqueIdentifierID) != 0) {
              (*context)->shouldParseTextNode = kEPUB3_NO; 
              EPUB3_FREE_AND_NULL(itemId);
            }
          }
        }

      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      const xmlChar *value = xmlTextReaderValue(reader);
      if(value != NULL && (*context)->shouldParseTextNode) {
        if(xmlStrcmp((*context)->tagName, BAD_CAST "title") == 0) {
          (void)EPUB3MetadataSetTitle(epub->metadata, (const char *)value);
        }
        else if(xmlStrcmp((*context)->tagName, BAD_CAST "identifier") == 0) {
          (void)EPUB3MetadataSetIdentifier(epub->metadata, (const char *)value);
        }
        else if(xmlStrcmp((*context)->tagName, BAD_CAST "language") == 0) {
          (void)EPUB3MetadataSetLanguage(epub->metadata, (const char *)value);
        }
      }
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ProcessXMLReaderNodeForManifestInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);

  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
      } else {
        if(xmlStrcmp(name, BAD_CAST "item") == 0) {
          EPUB3ManifestItemRef newItem = EPUB3ManifestItemCreate();
          newItem->itemId = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "id");
          newItem->href = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "href");
          newItem->mediaType = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "media-type");
          newItem->properties = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "properties");

          if(newItem->properties != NULL) {
            // Look for the cover-image property
            char *prop, *props, *tofree;
            tofree = props = strdup(newItem->properties);
            while((prop = strsep(&props, " ")) != NULL) {
              if(strcmp(prop, "cover-image") == 0) {
                EPUB3MetadataSetCoverImageId(epub->metadata, newItem->itemId);
              }
            }
            EPUB3_FREE_AND_NULL(tofree);
          }
          if(newItem->mediaType != NULL && strcmp(newItem->mediaType, "application/x-dtbncx+xml") == 0) {
            //This is the ref for the ncx document. Set it for v2 epubs
            if(epub->metadata->version == kEPUB3Version_2) {
              EPUB3MetadataSetNCXItem(epub->metadata, newItem);
            }
          }
          EPUB3ManifestInsertItem(epub->manifest, newItem);
        }
      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ProcessXMLReaderNodeForSpineInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);

  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
      } else {
        if(xmlStrcmp(name, BAD_CAST "itemref") == 0) {
          EPUB3SpineItemRef newItem = EPUB3SpineItemCreate();
          xmlChar * linear = xmlTextReaderGetAttribute(reader, BAD_CAST "linear");

          if(linear == NULL || xmlStrcmp(linear, BAD_CAST "yes") == 0) {
            newItem->isLinear = kEPUB3_YES;
            epub->spine->linearItemCount++;
          }
          EPUB3_FREE_AND_NULL(linear);
          newItem->idref = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "idref");
          if(newItem->idref != NULL) {
            EPUB3ManifestItemListItemPtr manifestPtr = EPUB3ManifestFindItemWithId(epub->manifest, newItem->idref);
            if(manifestPtr == NULL) {
              newItem->manifestItem = NULL;
            } else {
              newItem->manifestItem = manifestPtr->item;
            }
          }
          EPUB3SpineAppendItem(epub->spine, newItem);
        }
      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ParseXMLReaderNodeForOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *currentContext)
{
  assert(epub != NULL);
  assert(reader != NULL);
  assert(*currentContext != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes currentNodeType = xmlTextReaderNodeType(reader);

  if(name != NULL && currentNodeType != XML_READER_TYPE_COMMENT) {
    switch((*currentContext)->state)
    {
      case kEPUB3OPFStateRoot:
      {
//        fprintf(stdout, "ROOT: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_ELEMENT) {
          if(xmlStrcmp(name, BAD_CAST "package") == 0 && xmlTextReaderHasAttributes(reader)) {
            EPUB3_FREE_AND_NULL(epub->metadata->_uniqueIdentifierID);
            epub->metadata->_uniqueIdentifierID = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "unique-identifier");
            xmlChar *versionString = xmlTextReaderGetAttribute(reader, BAD_CAST "version");
            if(versionString != NULL) {
              if(*versionString == '2') {
                epub->metadata->version = kEPUB3Version_2;
              } else if(*versionString == '3') {
                epub->metadata->version = kEPUB3Version_3;
              }
              EPUB3_FREE_AND_NULL(versionString);
            }
          }
          else if(xmlStrcmp(name, BAD_CAST "metadata") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "manifest") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "spine") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateSpine, name, 0, NULL, kEPUB3_YES);
          }
        }
        break;
      }
      case kEPUB3OPFStateMetadata:
      {
//        fprintf(stdout, "METADATA: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "metadata") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForMetadataInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateManifest:
      {
//        fprintf(stdout, "MANIFEST: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "manifest") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForManifestInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateSpine:
      {
//        fprintf(stdout, "SPINE: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "spine") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForSpineInOPF(epub, reader, currentContext);
        }
        break;
      }
      default: break;
    }
  }
  return error;
}

EPUB3Error EPUB3ParseOPFFromData(EPUB3Ref epub, void * buffer, uint32_t bufferSize)
{
  assert(epub != NULL);
  assert(buffer != NULL);
  assert(bufferSize > 0);

  EPUB3Error error = kEPUB3Success;
  xmlInitParser();
  xmlTextReaderPtr reader = NULL;
  reader = xmlReaderForMemory(buffer, bufferSize, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
  if(reader != NULL) {
    EPUB3XMLParseContext contextStack[PARSE_CONTEXT_STACK_DEPTH];
    EPUB3XMLParseContextPtr currentContext = &contextStack[0];

    int retVal = xmlTextReaderRead(reader);
    currentContext->state = kEPUB3OPFStateRoot;
    currentContext->tagName = xmlTextReaderConstName(reader);
    while(retVal == 1)
    {
      error = EPUB3ParseXMLReaderNodeForOPF(epub, reader, &currentContext);
      retVal = xmlTextReaderRead(reader);
    }
    if(retVal < 0) {
      error = kEPUB3XMLParseError;
    }
  } else {
    error = kEPUB3XMLReadFromBufferError;
  }
  xmlFreeTextReader(reader);
  xmlCleanupParser();
  return error;
}

#pragma mark - NCX XML Parsing

// TODO: Refactor: This function differs from EPUB3ParseOPFFromData by 1 line.
EPUB3Error EPUB3ParseNCXFromData(EPUB3Ref epub, void * buffer, uint32_t bufferSize)
{
  assert(epub != NULL);
  assert(buffer != NULL);
  assert(bufferSize > 0);

  EPUB3Error error = kEPUB3Success;
  xmlInitParser();
  xmlTextReaderPtr reader = NULL;
  reader = xmlReaderForMemory(buffer, bufferSize, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
  if(reader != NULL) {
    EPUB3XMLParseContext contextStack[PARSE_CONTEXT_STACK_DEPTH];
    EPUB3XMLParseContextPtr currentContext = &contextStack[0];

    int retVal = xmlTextReaderRead(reader);
    currentContext->state = kEPUB3OPFStateRoot;
    currentContext->tagName = xmlTextReaderConstName(reader);
    while(retVal == 1)
    {
      error = EPUB3ParseXMLReaderNodeForNCX(epub, reader, &currentContext);
      retVal = xmlTextReaderRead(reader);
    }
    if(retVal < 0) {
      error = kEPUB3XMLParseError;
    }
  } else {
    error = kEPUB3XMLReadFromBufferError;
  }
  xmlFreeTextReader(reader);
  xmlCleanupParser();
  return error;
}

EPUB3Error EPUB3ParseXMLReaderNodeForNCX(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *currentContext)
{
  assert(epub != NULL);
  assert(reader != NULL);
  assert(*currentContext != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes currentNodeType = xmlTextReaderNodeType(reader);

  if(name != NULL && currentNodeType != XML_READER_TYPE_COMMENT) {
    switch((*currentContext)->state)
    {
      case kEPUB3OPFStateRoot:
      {
        fprintf(stdout, "NCX ROOT: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_ELEMENT) {
          if(xmlStrcmp(name, BAD_CAST "navMap") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3NCXStateNavMap, name, 0, NULL, kEPUB3_YES);
          }
        }
        break;
      }
      case kEPUB3NCXStateNavMap:
      {
        fprintf(stdout, "NCX NAV MAP: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "navMap") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForNavMapInNCX(epub, reader, currentContext);
        }
        break;
      }
      default: break;
    }
  }
  return error;
}

EPUB3Error EPUB3ProcessXMLReaderNodeForNavMapInNCX(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3XMLParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
//  const xmlChar *name = xmlTextReaderConstLocalName(reader);
//  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);
//
//  switch(nodeType)
//  {
//    case XML_READER_TYPE_ELEMENT:
//    {
//      if(!xmlTextReaderIsEmptyElement(reader)) {
//        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);
//
//        // Only parse text node for the identifier marked as unique-identifier in the package tag
//        // see: http://idpf.org/epub/30/spec/epub30-publications.html#sec-opf-dcidentifier
//        if(xmlStrcmp(name, BAD_CAST "identifier") == 0) {
//          if(xmlTextReaderHasAttributes(reader)) {
//            xmlChar * itemId = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
//            if(itemId == NULL) {
//              (*context)->shouldParseTextNode = kEPUB3_NO;
//            }
//            else if(itemId != NULL && xmlStrcmp(itemId, BAD_CAST epub->metadata->_uniqueIdentifierID) != 0) {
//              (*context)->shouldParseTextNode = kEPUB3_NO; 
//              EPUB3_FREE_AND_NULL(itemId);
//            }
//          }
//        }
//
//      }
//      break;
//    }
//    case XML_READER_TYPE_TEXT:
//    {
//      const xmlChar *value = xmlTextReaderValue(reader);
//      if(value != NULL && (*context)->shouldParseTextNode) {
//        if(xmlStrcmp((*context)->tagName, BAD_CAST "title") == 0) {
//          (void)EPUB3MetadataSetTitle(epub->metadata, (const char *)value);
//        }
//        else if(xmlStrcmp((*context)->tagName, BAD_CAST "identifier") == 0) {
//          (void)EPUB3MetadataSetIdentifier(epub->metadata, (const char *)value);
//        }
//        else if(xmlStrcmp((*context)->tagName, BAD_CAST "language") == 0) {
//          (void)EPUB3MetadataSetLanguage(epub->metadata, (const char *)value);
//        }
//      }
//      break;
//    }
//    case XML_READER_TYPE_END_ELEMENT:
//    {
//      (void)EPUB3PopAndFreeParseContext(context);
//      break;
//    }
//    default: break;
//  }
  return error;
}


#pragma mark - Validation

EPUB3Error EPUB3ValidateMimetype(EPUB3Ref epub)
{
  assert(epub != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error status = kEPUB3InvalidMimetypeError;
  static const char * requiredMimetype = "application/epub+zip";
  static const int stringLength = 20;
  char buffer[stringLength];

  if(unzGoToFirstFile(epub->archive) == UNZ_OK) {
    uint32_t stringLength = (uint32_t)strlen(requiredMimetype);
    if(unzOpenCurrentFile(epub->archive) == UNZ_OK) {
      int byteCount = unzReadCurrentFile(epub->archive, buffer, stringLength);
      if(byteCount == stringLength) {
        if(strncmp(requiredMimetype, buffer, stringLength) == 0) {
          status = kEPUB3Success;
        }
      }
    }
    unzCloseCurrentFile(epub->archive);
  }
  return status;
}

EXPORT EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath)
{
  assert(epub != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  static const char *containerFilename = "META-INF/container.xml";

  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;

  xmlTextReaderPtr reader = NULL;
  EPUB3Bool foundPath = kEPUB3_NO;

  EPUB3Error error = kEPUB3Success;

  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, containerFilename);
  if(error == kEPUB3Success) {
    reader = xmlReaderForMemory(buffer, bufferSize, "", NULL, XML_PARSE_RECOVER);
    if(reader != NULL) {
      int retVal;
      while((retVal = xmlTextReaderRead(reader)) == 1)
      {
        const char *rootFileName = "rootfile";
        const xmlChar *name = xmlTextReaderConstLocalName(reader);

        if(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlStrcmp(name, BAD_CAST rootFileName) == 0) {
          xmlChar *fullPath = xmlTextReaderGetAttribute(reader, BAD_CAST "full-path");
          if(fullPath != NULL) {
            // TODD: validate that the full-path attribute is of the form path-rootless
            //       see http://idpf.org/epub/30/spec/epub30-ocf.html#sec-container-metainf-container.xml
            foundPath = kEPUB3_YES;
            *rootPath = strdup((char *)fullPath);
          } else {
            // The spec requires the full-path attribute
            error = kEPUB3XMLXDocumentInvalidError;
          }
          break;
        }
      }
      if(retVal < 0) {
        error = kEPUB3XMLParseError;
      }
      if(!foundPath) {
        error = kEPUB3XMLXElementNotFoundError;
      }
    } else {
      error = kEPUB3XMLReadFromBufferError;
    }
    EPUB3_FREE_AND_NULL(buffer);
  }
  xmlFreeTextReader(reader);
  return error;
}

EPUB3Error EPUB3ValidateFileExistsAndSeekInArchive(EPUB3Ref epub, const char * filename)
{
  assert(epub != NULL);
  assert(filename != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error error = kEPUB3FileNotFoundInArchiveError;
  if(unzLocateFile(epub->archive, filename, 1) == UNZ_OK) {
    error = kEPUB3Success;
  }
  return error;
}

#pragma mark - Utility functions

// This buffer size was chosen because it matches UNZ_BUFSIZE in unzip.c
#define FILE_EXTRACT_BUFFER_SIZE (16384)

EXPORT EPUB3Error EPUB3ExtractArchiveToPath(EPUB3Ref epub, const char * path)
{
  assert(epub != NULL);
  assert(path != NULL);

  EPUB3Error error = kEPUB3UnknownError;

  char cwd[MAXNAMLEN];
  (void)getcwd(cwd, MAXNAMLEN);

  EPUB3Bool directoryReady = (chdir(path) >= 0);

  if(!directoryReady) {
    if(errno == ENOENT) {
      if(mkdir(path, 0755) < 0) {
        fprintf(stderr, "Error [%d] creating directory %s\n", errno, path);
        error = kEPUB3UnknownError;
      } else {
        if(chdir(path) < 0) {
          fprintf(stderr, "Error [%d] changing to newly created dir %s\n", errno, path);
        } else {
          directoryReady = kEPUB3_YES;
        }
      }
    } else {
      fprintf(stderr, "Error [%d] opening %s\n", errno, path);
      error = kEPUB3UnknownError;
    }
  }

  if(directoryReady) {
    if(unzGoToFirstFile(epub->archive) == UNZ_OK) {
      int fileCount = 0;
      do {
        error = EPUB3WriteCurrentArchiveFileToPath(epub, path);
        if(error == kEPUB3Success) {
          fileCount++;
        }
      } while(unzGoToNextFile(epub->archive) == UNZ_OK);
      if(fileCount == EPUB3GetFileCountInArchive(epub)) {
        error = kEPUB3Success;
      }
    }
  }

  if(chdir(cwd) < 0) {
    fprintf(stderr, "Error [%d] changing back to starting dir %s\n", errno, cwd);
  }
  return error;
}

EPUB3Error EPUB3CreateNestedDirectoriesForFileAtPath(const char * path)
{
  EPUB3Error error = kEPUB3Success;
  char * pathCopy = strdup(path);
  char pathBuildup[strlen(path) + 1];
  pathBuildup[0] = '\0';
  char * pathseg;
  char * pathseg2;
  char * loc;

  pathseg = strtok_r(pathCopy, "/", &loc);

  while(pathseg != NULL) {
    pathseg2 = strtok_r(NULL, "/", &loc);
    if(pathseg2 != NULL) {
      strncat(pathBuildup, "/", 1U);
      strncat(pathBuildup, pathseg, strlen(pathseg));
      struct stat st;
      if(stat(pathBuildup, &st) < 0) {
        if(errno == ENOENT) {
          //Directory doesn't exist
          if(mkdir(pathBuildup, 0755) >= 0) {
            error = kEPUB3Success;
          } else {
            // Couldn't create dir
            error = kEPUB3UnknownError;
            break;
          }
        } else {
          // Weird stat error
          error = kEPUB3UnknownError;
          break;
        }
      } else {
        // Already exists
        error = kEPUB3Success;
      }
    }
    pathseg = pathseg2;
  }
  EPUB3_FREE_AND_NULL(pathCopy);
  return error;
}

EPUB3Error EPUB3WriteCurrentArchiveFileToPath(EPUB3Ref epub, const char * path)
{
  EPUB3Error error = kEPUB3Success;
  unz_file_info fileInfo;
  char filename[MAXNAMLEN];
  if(unzGetCurrentFileInfo(epub->archive, &fileInfo, filename, MAXNAMLEN, NULL, 0, NULL, 0) == UNZ_OK) {
    uLong pathlen = strlen(path) + 1U + strlen(filename) + 1U;
    char fullpath[pathlen];
    (void)strcpy(fullpath, path);
    (void)strncat(fullpath, "/", 1U);
    (void)strncat(fullpath, filename, strlen(filename));

    FILE *destination = fopen(fullpath, "wb");
    if(destination == NULL) {
      if(errno == ENOENT) {
        //We need to create intermediate directories
        error = EPUB3CreateNestedDirectoriesForFileAtPath(fullpath);
        if(error == kEPUB3Success) {
          //Try again
          if((destination = fopen(fullpath, "wb")) == NULL) {
            //Failed again can't continue
            error = kEPUB3UnknownError;
          }
        }
      } else {
        error = kEPUB3UnknownError;
      }
    }
    if(destination != NULL) {
      void *buffer = malloc(FILE_EXTRACT_BUFFER_SIZE);
      if(unzOpenCurrentFile(epub->archive) == UNZ_OK) {
        int bytesRead;
        do {
          bytesRead = unzReadCurrentFile(epub->archive, buffer, FILE_EXTRACT_BUFFER_SIZE);
          if(bytesRead < 0) {
            error = kEPUB3FileReadFromArchiveError;
            break;
          } else {
            fwrite(buffer, 1, bytesRead, destination);
          }
        } while(bytesRead > 0);
        unzCloseCurrentFile(epub->archive);
      }
      fclose(destination);
      EPUB3_FREE_AND_NULL(buffer);
    }
  }
  return error;
}

EPUB3Error EPUB3CopyFileIntoBuffer(EPUB3Ref epub, void **buffer, uint32_t *bufferSize, uint32_t *bytesCopied, const char * filename)
{
  assert(epub != NULL);
  assert(filename != NULL);
  assert(buffer != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error error = kEPUB3InvalidArgumentError;
  if(filename != NULL) {
    uint32_t bufSize = 1;
    error = EPUB3GetUncompressedSizeOfFileInArchive(epub, &bufSize, filename);
    if(error == kEPUB3Success) {
      if(unzOpenCurrentFile(epub->archive) == UNZ_OK) {
        *buffer = calloc(bufSize, sizeof(char));
        int32_t copied = unzReadCurrentFile(epub->archive, *buffer, bufSize);
        if(copied >= 0) {
          if(bytesCopied != NULL) {
            *bytesCopied = copied;
          }
          if(bufferSize != NULL) {
            *bufferSize = bufSize;
          }
          error = kEPUB3Success;
        } else {
          free(*buffer);
          error = kEPUB3FileReadFromArchiveError;
        }
      }
    }
  }
  return error;
}

EPUB3Error EPUB3GetUncompressedSizeOfFileInArchive(EPUB3Ref epub, uint32_t *uncompressedSize, const char *filename)
{
  assert(epub != NULL);
  assert(filename != NULL);
  assert(uncompressedSize != NULL);

  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error error = EPUB3ValidateFileExistsAndSeekInArchive(epub, filename);
  if(error == kEPUB3Success) {
    unz_file_info fileInfo;
    if(unzGetCurrentFileInfo(epub->archive, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
      *uncompressedSize = (uint32_t)fileInfo.uncompressed_size;
      error = kEPUB3Success;
    }
  }
  return error;
}

uint32_t EPUB3GetFileCountInArchive(EPUB3Ref epub)
{
  unz_global_info gi;
	int err = unzGetGlobalInfo(epub->archive, &gi);
	if (err != UNZ_OK)
    return err;

	return (uint32_t)gi.number_entry;
}

char * EPUB3CopyOfPathByDeletingLastPathComponent(const char * path)
{
  assert(path != NULL);

  char * pathCopy = strdup(path);
  char pathBuildup[strlen(path) + 1];
  pathBuildup[0] = '\0';
  char * pathseg;
  char * pathseg2;
  char * loc;

  pathseg = strtok_r(pathCopy, "/", &loc);

  while(pathseg != NULL) {
    pathseg2 = strtok_r(NULL, "/", &loc);
    if(pathseg2 != NULL) {
      strncat(pathBuildup, pathseg, strlen(pathseg));
      strncat(pathBuildup, "/", 1U);
    }
    pathseg = pathseg2;
  }
  EPUB3_FREE_AND_NULL(pathCopy);

  return strdup(pathBuildup);
}

char * EPUB3CopyOfPathByAppendingPathComponent(const char * path, const char * componentToAppend)
{
  assert(path != NULL);
  assert(componentToAppend != NULL);

  uLong basePathLen = strlen(path);

  EPUB3Bool shouldAddSeparator = kEPUB3_NO;

  if(path[basePathLen - 1] != '/') {
    shouldAddSeparator = kEPUB3_YES;
    basePathLen++;
  }

  uLong pathlen = basePathLen + strlen(componentToAppend) + 1U;
  char fullpath[pathlen];
  (void)strcpy(fullpath, path);
  if(shouldAddSeparator) {
    (void)strncat(fullpath, "/", 1U);
  }
  (void)strncat(fullpath, componentToAppend, strlen(componentToAppend));
  return strdup(fullpath);
}

