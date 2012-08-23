#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";

#ifndef PARSE_CONTEXT_STACK_DEPTH
#define PARSE_CONTEXT_STACK_DEPTH 64
#endif

#pragma mark - Memory Management (Reference Counting)

void _EPUB3ObjectRelease(void *object)
{
  if(object == NULL) return;

  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount -= 1;
  if(obj->_type.refCount == 0) {
    free(obj);
  }
}

void _EPUB3ObjectRetain(void *object)
{
  if(object == NULL) return;
  
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount += 1;
}

void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID)
{
  assert(object != NULL);
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.typeID = typeID;
  obj->_type.refCount = 1;
  return obj;
}

EXPORT void EPUB3Retain(EPUB3Ref epub)
{
  if(epub == NULL) return;
  
  EPUB3MetadataRetain(epub->metadata);
  _EPUB3ObjectRetain(epub);
}

EXPORT void EPUB3Release(EPUB3Ref epub)
{
  if(epub == NULL) return;

  EPUB3MetadataRelease(epub->metadata);
  if(epub->archive != NULL) {
    unzClose(epub->archive);
    epub->archive = NULL;
  }
  free(epub->archivePath);
  _EPUB3ObjectRelease(epub);
  xmlCleanupParser();
}

EXPORT void EPUB3MetadataRetain(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;
  
  _EPUB3ObjectRetain(metadata);
}

EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;

  free(metadata->title);
  free(metadata->_uniqueIdentifierID);
  free(metadata->identifier);
  free(metadata->language);

  _EPUB3ObjectRelease(metadata);
}

#pragma mark - EPUB3Ref

EPUB3Ref EPUB3Create()
{
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = _EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  memory->archive = NULL;
  memory->archivePath = NULL;
  memory->archiveFileCount = 0;
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
  epub->metadata = NULL;
  xmlInitParser();
  //TODO: parse and load metadata?
  return epub;
}


#pragma mark - EPUB3MetadataRef

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

EPUB3Error _EPUB3ProcessXMLReaderNodeForMetadataInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseStateContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);
  int depth = xmlTextReaderDepth(reader);

  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (*context)++;
        (*context)->state = kEPUB3OPFStateMetadata;
        (*context)->tagName = name;
        (*context)->shouldParseTextNode = kEPUB3_YES;
        (*context)->depth = depth;

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
      (*context)--;
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error _EPUB3ParseXMLReaderNodeForOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseStateContextPtr *currentContext)
{
  assert(epub != NULL);
  assert(reader != NULL);
  assert(*currentContext != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  int depth = xmlTextReaderDepth(reader);
  xmlReaderTypes currentNodeType = xmlTextReaderNodeType(reader);
  
  if(name != NULL && currentNodeType != XML_READER_TYPE_COMMENT) {
    switch((*currentContext)->state)
    {
      case kEPUB3OPFStateRoot:
      {
        if(currentNodeType == XML_READER_TYPE_ELEMENT) {
          if(xmlStrcmp(name, BAD_CAST "package") == 0 && xmlTextReaderHasAttributes(reader)) {
            if(epub->metadata->_uniqueIdentifierID != NULL) {
              free(epub->metadata->_uniqueIdentifierID);
            }
            epub->metadata->_uniqueIdentifierID = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "unique-identifier");
          }
          else if(xmlStrcmp(name, BAD_CAST "metadata") == 0) {
            (*currentContext)++;
            (*currentContext)->state = kEPUB3OPFStateMetadata;
            (*currentContext)->tagName = name;
            (*currentContext)->shouldParseTextNode = kEPUB3_YES;
            (*currentContext)->depth = depth;
          }
          else if(xmlStrcmp(name, BAD_CAST "manifest") == 0) {
            (*currentContext)++;
            (*currentContext)->state = kEPUB3OPFStateManifest;
            (*currentContext)->tagName = name;
            (*currentContext)->shouldParseTextNode = kEPUB3_YES;
            (*currentContext)->depth = depth;
          }
          else if(xmlStrcmp(name, BAD_CAST "spine") == 0) {
            (*currentContext)++;
            (*currentContext)->state = kEPUB3OPFStateSpine;
            (*currentContext)->tagName = name;
            (*currentContext)->shouldParseTextNode = kEPUB3_YES;
            (*currentContext)->depth = depth;
          }
        }
        break;
      }
      case kEPUB3OPFStateMetadata:
      {
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "metadata") == 0) {
          (*currentContext)--;
        } else {
          error = _EPUB3ProcessXMLReaderNodeForMetadataInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateManifest:
      {
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "manifest") == 0) {
          (*currentContext)--;
        } else {

        }
        break;
      }
      case kEPUB3OPFStateSpine:
      {
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "spine") == 0) {
          (*currentContext)--;
        } else {

        }
        break;
      }
      default: break;
    }
  }
  return error;
}

EPUB3Error EPUB3InitMetadataFromOPF(EPUB3Ref epub, const char * opfFilename)
{
  assert(epub != NULL);
  assert(opfFilename != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  if(epub->metadata == NULL) {
    epub->metadata = EPUB3MetadataCreate();
  }

  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;
  
  xmlTextReaderPtr reader = NULL;
  
  EPUB3Error error = kEPUB3Success;
  
  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, opfFilename);
  if(error == kEPUB3Success) {
    reader = xmlReaderForMemory(buffer, bufferSize, "", NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
    // (void)xmlTextReaderSetParserProp(reader, XML_PARSER_VALIDATE, 1);
    if(reader != NULL) {
      EPUB3OPFParseStateContext contextStack[PARSE_CONTEXT_STACK_DEPTH];
      EPUB3OPFParseStateContextPtr currentContext = &contextStack[0];

      int retVal = xmlTextReaderRead(reader);
      currentContext->state = kEPUB3OPFStateRoot;
      currentContext->tagName = xmlTextReaderConstName(reader);
      currentContext->depth = xmlTextReaderDepth(reader);
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
    free(buffer);
  }
  xmlFreeTextReader(reader);
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
