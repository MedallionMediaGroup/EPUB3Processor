#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";
const char * kEPUB3ManifestTypeID = "_EPUB3Manifest_t";
const char * kEPUB3ManifestItemTypeID = "_EPUB3ManifestItem_t";

#ifndef PARSE_CONTEXT_STACK_DEPTH
#define PARSE_CONTEXT_STACK_DEPTH 64
#endif

#pragma mark - Base Object

void _EPUB3ObjectRelease(void *object)
{
  if(object == NULL) return;

  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount--;
  if(obj->_type.refCount == 0) {
    free(obj);
  }
}

void _EPUB3ObjectRetain(void *object)
{
  if(object == NULL) return;
  
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount++;
}

void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID)
{
  assert(object != NULL);
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.typeID = typeID;
  obj->_type.refCount = 1;
  return obj;
}

#pragma mark - Main EPUB3 Object

EXPORT void EPUB3Retain(EPUB3Ref epub)
{
  if(epub == NULL) return;
  
  EPUB3MetadataRetain(epub->metadata);
  _EPUB3ObjectRetain(epub);
}

EXPORT void EPUB3Release(EPUB3Ref epub)
{
  if(epub == NULL) return;
  
  if(epub->_type.refCount == 1) {
    if(epub->archive != NULL) {
      unzClose(epub->archive);
      epub->archive = NULL;
    }
    free(epub->archivePath);
    xmlCleanupParser();
  }

  EPUB3MetadataRelease(epub->metadata);
  EPUB3ManifestRelease(epub->manifest);
  _EPUB3ObjectRelease(epub);
}

void EPUB3SetStringValue(char ** location, const char *value)
{
  if(*location != NULL) {
    free(*location);
  }
  if(value == NULL) {
    *location = NULL;
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

#pragma mark - Metadata

EXPORT void EPUB3MetadataRetain(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;
  
  _EPUB3ObjectRetain(metadata);
}

EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;

  if(metadata->_type.refCount == 1) {
    free(metadata->title);
    free(metadata->_uniqueIdentifierID);
    free(metadata->identifier);
    free(metadata->language);
  }
  _EPUB3ObjectRelease(metadata);
}

EPUB3MetadataRef EPUB3MetadataCreate()
{
  EPUB3MetadataRef memory = malloc(sizeof(struct EPUB3Metadata));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3MetadataTypeID);
  memory->title = NULL;
  memory->_uniqueIdentifierID = NULL;
  memory->identifier = NULL;
  memory->language = NULL;
  return memory;
}

EXPORT void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->title), title);
}

EXPORT char * EPUB3CopyMetadataTitle(EPUB3MetadataRef metadata)
{
  assert(metadata != NULL);
  return EPUB3CopyStringValue(&(metadata->title));
}

EXPORT void EPUB3MetadataSetIdentifier(EPUB3MetadataRef metadata, const char * identifier)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->identifier), identifier);
}

EXPORT char * EPUB3CopyMetadataIdentifier(EPUB3MetadataRef metadata)
{
  assert(metadata != NULL);
  return EPUB3CopyStringValue(&(metadata->identifier));
}

EXPORT void EPUB3MetadataSetLanguage(EPUB3MetadataRef metadata, const char * language)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->language), language);
}

EXPORT char * EPUB3CopyMetadataLanguage(EPUB3MetadataRef metadata)
{
  assert(metadata != NULL);
  return EPUB3CopyStringValue(&(metadata->language));
}

#pragma mark - Manifest

EXPORT void EPUB3ManifestRetain(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;

  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    EPUB3ManifestItemListPtr itemPtr = manifest->itemTable[i];
    while(itemPtr != NULL) {
      EPUB3ManifestItemRetain(itemPtr->item);
      itemPtr = itemPtr->next;
    }
  }
  _EPUB3ObjectRetain(manifest);
}

EXPORT void EPUB3ManifestRelease(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {

    EPUB3ManifestItemListPtr next = manifest->itemTable[i];
    while(next != NULL) {
      EPUB3ManifestItemRelease(next->item);
      EPUB3ManifestItemListPtr tmp = next;
      next = tmp->next;
      free(tmp);
    }
    manifest->itemTable[i] = NULL;
  }
  manifest->itemCount = 0;
  _EPUB3ObjectRelease(manifest);
}

EXPORT void EPUB3ManifestItemRetain(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;
  
  _EPUB3ObjectRetain(item);
}

EXPORT void EPUB3ManifestItemRelease(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;

  if(item->_type.refCount == 1) {
    free(item->id);
    free(item->href);
    free(item->mediaType);
    free(item->properties);
  }
  
  _EPUB3ObjectRelease(item);
}

EPUB3ManifestRef EPUB3ManifestCreate()
{
  EPUB3ManifestRef memory = malloc(sizeof(struct EPUB3Manifest));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestTypeID);
  memory->itemCount = 0;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    memory->itemTable[i] = NULL;
  }
  return memory;
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


EPUB3ManifestItemRef EPUB3ManifestItemCreate()
{
  EPUB3ManifestItemRef memory = malloc(sizeof(struct EPUB3ManifestItem));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestItemTypeID);
  memory->id= NULL;
  memory->href= NULL;
  memory->mediaType= NULL;
  memory->properties= NULL;
  return memory;
}

void EPUB3ManifestInsertItem(EPUB3ManifestRef manifest, EPUB3ManifestItemRef item)
{
  assert(manifest != NULL);
  assert(item != NULL);
  assert(item->id != NULL);

  EPUB3ManifestItemRetain(item);
  EPUB3ManifestItemListPtr itemPtr = _EPUB3ManifestFindItemWithId(manifest, item->id);
  if(itemPtr == NULL) {
    itemPtr = (EPUB3ManifestItemListPtr) malloc(sizeof(struct EPUB3ManifestItemList));
    int32_t bucket = SuperFastHash(item->id, (int32_t)strlen(item->id)) % MANIFEST_HASH_SIZE;
    itemPtr->item = item;
    itemPtr->next = manifest->itemTable[bucket];
    manifest->itemTable[bucket] = itemPtr;
    manifest->itemCount++;
  } else {
    EPUB3ManifestItemRelease(itemPtr->item);
    itemPtr->item = item;
  }
}

EPUB3ManifestItemRef EPUB3ManifestCopyItemWithId(EPUB3ManifestRef manifest, const char * id)
{
  assert(manifest != NULL);
  assert(id != NULL);

  EPUB3ManifestItemListPtr itemPtr = _EPUB3ManifestFindItemWithId(manifest, id);

  EPUB3ManifestItemRef item = itemPtr->item;
  EPUB3ManifestItemRef copy = EPUB3ManifestItemCreate();
  copy->id = item->id != NULL ? strdup(item->id) : NULL;
  copy->href = item->href != NULL ? strdup(item->href) : NULL;
  copy->mediaType = item->mediaType != NULL ? strdup(item->mediaType) : NULL;
  copy->properties = item->properties != NULL ? strdup(item->properties) : NULL;
  return copy;
}

EPUB3ManifestItemListPtr _EPUB3ManifestFindItemWithId(EPUB3ManifestRef manifest, const char * id)
{
  assert(manifest != NULL);
  assert(id != NULL);

  int32_t bucket = SuperFastHash(id, (int32_t)strlen(id)) % MANIFEST_HASH_SIZE; 
  EPUB3ManifestItemListPtr itemPtr = manifest->itemTable[bucket];
  while(itemPtr != NULL) {
    if(strcmp(id, itemPtr->item->id) == 0) {
      return itemPtr;
    }
    itemPtr = itemPtr->next;
  }
  return NULL;
}


#pragma mark - EPUB3Ref

EPUB3Ref EPUB3Create()
{
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  memory->manifest = NULL;
  memory->archive = NULL;
  memory->archivePath = NULL;
  memory->archiveFileCount = 0;
  //TODO: find a better place for the xmlInitParser() call
  xmlInitParser();
  return memory;
}

EXPORT EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path)
{
  assert(path != NULL);
  
  EPUB3Ref epub = EPUB3Create();
  unzFile archive = unzOpen(path);
  epub->archive = archive;
  epub->archiveFileCount = _GetFileCountInZipFile(archive);
  epub->archivePath = strdup(path);
  //TODO: parse and load metadata here?
  return epub;
}

EXPORT EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub)
{
  assert(epub != NULL);
  
  if(epub->metadata == NULL) {
    return NULL;
  }
  EPUB3MetadataRef copy = EPUB3MetadataCreate();
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

#pragma mark - XML Parsing Utilities

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

void _EPUB3SaveParseContext(EPUB3OPFParseContextPtr *ctxPtr, EPUB3OPFParseState state, const xmlChar * tagName, int32_t attrCount, char ** attrs, EPUB3Bool shouldParseTextNode)
{
  (*ctxPtr)++;
  (*ctxPtr)->state = state;
  (*ctxPtr)->tagName = tagName;
  (*ctxPtr)->attributeCount = attrCount;
  (*ctxPtr)->attributes = attrs;
  (*ctxPtr)->shouldParseTextNode = shouldParseTextNode;
}

void _EPUB3PopAndFreeParseContext(EPUB3OPFParseContextPtr *contextPtr)
{
  EPUB3OPFParseContextPtr ctx = (*contextPtr);
  (*contextPtr)--;
  for (int i = 0; i < ctx->attributeCount; i++) {
    char * key = ctx->attributes[i * 2];
    char * val = ctx->attributes[i * 2 + 1];
    free(key);
    free(val);
  }
}

EPUB3Error _EPUB3ProcessXMLReaderNodeForMetadataInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context)
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
        (void)_EPUB3SaveParseContext(context, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);

        // Only parse text node for the identifier marked as unique-identifier in the package tag
        // see: http://idpf.org/epub/30/spec/epub30-publications.html#sec-opf-dcidentifier
        if(xmlStrcmp(name, BAD_CAST "identifier") == 0) {
          if(xmlTextReaderHasAttributes(reader)) {
            xmlChar * id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
            if(id == NULL) {
              (*context)->shouldParseTextNode = kEPUB3_NO;
            }
            else if(id != NULL && xmlStrcmp(id, BAD_CAST epub->metadata->_uniqueIdentifierID) != 0) {
              (*context)->shouldParseTextNode = kEPUB3_NO; 
              free(id);
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
      (void)_EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error _EPUB3ProcessXMLReaderNodeForManifestInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context)
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
        (void)_EPUB3SaveParseContext(context, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
      } else {
        if(xmlStrcmp(name, BAD_CAST "item") == 0) {
          EPUB3ManifestItemRef newItem = EPUB3ManifestItemCreate();
          newItem->id = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "id");
          newItem->href = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "href");
          newItem->mediaType = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "media-type");
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
      (void)_EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error _EPUB3ParseXMLReaderNodeForOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *currentContext)
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
        fprintf(stdout, "ROOT: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_ELEMENT) {
          if(xmlStrcmp(name, BAD_CAST "package") == 0 && xmlTextReaderHasAttributes(reader)) {
            if(epub->metadata->_uniqueIdentifierID != NULL) {
              free(epub->metadata->_uniqueIdentifierID);
            }
            epub->metadata->_uniqueIdentifierID = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "unique-identifier");
          }
          else if(xmlStrcmp(name, BAD_CAST "metadata") == 0) {
            (void)_EPUB3SaveParseContext(currentContext, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "manifest") == 0) {
            (void)_EPUB3SaveParseContext(currentContext, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "spine") == 0) {
            (void)_EPUB3SaveParseContext(currentContext, kEPUB3OPFStateSpine, name, 0, NULL, kEPUB3_YES);
          }
        }
        break;
      }
      case kEPUB3OPFStateMetadata:
      {
        fprintf(stdout, "METADATA: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "metadata") == 0) {
          (void)_EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = _EPUB3ProcessXMLReaderNodeForMetadataInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateManifest:
      {
        fprintf(stdout, "MANIFEST: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "manifest") == 0) {
          (void)_EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = _EPUB3ProcessXMLReaderNodeForManifestInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateSpine:
      {
        fprintf(stdout, "SPINE: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "spine") == 0) {
          (void)_EPUB3PopAndFreeParseContext(currentContext);
        } else {

        }
        break;
      }
      default: break;
    }
  }
  return error;
}

EPUB3Error _EPUB3ParseFromOPFData(EPUB3Ref epub, void * buffer, uint32_t bufferSize)
{
  assert(epub != NULL);
  assert(buffer != NULL);
  assert(bufferSize > 0);

  EPUB3Error error = kEPUB3Success;
  xmlTextReaderPtr reader = NULL;
  reader = xmlReaderForMemory(buffer, bufferSize, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
  // (void)xmlTextReaderSetParserProp(reader, XML_PARSER_VALIDATE, 1);
  if(reader != NULL) {
    EPUB3OPFParseContext contextStack[PARSE_CONTEXT_STACK_DEPTH];
    EPUB3OPFParseContextPtr currentContext = &contextStack[0];

    int retVal = xmlTextReaderRead(reader);
    currentContext->state = kEPUB3OPFStateRoot;
    currentContext->tagName = xmlTextReaderConstName(reader);
    while(retVal == 1)
    {
      error = _EPUB3ParseXMLReaderNodeForOPF(epub, reader, &currentContext);
      retVal = xmlTextReaderRead(reader);
    }
    if(retVal < 0) {
      error = kEPUB3XMLParseError;
    }
  } else {
    error = kEPUB3XMLReadFromBufferError;
  }
  xmlFreeTextReader(reader);
  return error;
}

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
  
  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;
  
  EPUB3Error error = kEPUB3Success;
  
  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, opfFilename);
  if(error == kEPUB3Success) {
    error = _EPUB3ParseFromOPFData(epub, buffer, bufferSize);
    free(buffer);
  }
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

EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath)
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
            *rootPath = (char *)fullPath;
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
    free(buffer);
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

uint32_t _GetFileCountInZipFile(unzFile file)
{
  unz_global_info gi;
	int err = unzGetGlobalInfo(file, &gi);
	if (err != UNZ_OK)
    return err;
	
	return (uint32_t)gi.number_entry;
}
