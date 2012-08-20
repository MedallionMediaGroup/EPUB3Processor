#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";

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
  return memory;
}

EXPORT EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub)
{
  assert(epub != NULL);
  
  if(epub->metadata == NULL) {
    return NULL;
  }
  EPUB3MetadataRef copy = EPUB3MetadataCreate();
  EPUB3MetadataSetTitle(copy, epub->metadata->title);
  return copy;
}

EXPORT void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title)
{
  assert(metadata != NULL);

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

EXPORT char * EPUB3CopyMetadataTitle(EPUB3MetadataRef metadata)
{
  assert(metadata != NULL);

  char * copy = strdup(metadata->title);
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
    if(error == kEPUB3Success)
    {
      if(unzOpenCurrentFile(epub->archive) == UNZ_OK)
      {
        *buffer = calloc(bufSize, sizeof(char));
        int32_t copied = unzReadCurrentFile(epub->archive, *buffer, bufSize);
        if(copied >= 0)
        {
          if(bytesCopied != NULL) {
            *bytesCopied = copied;
          }
          if(bufferSize != NULL) {
            *bufferSize = bufSize;
          }
          error = kEPUB3Success;
        }
        else
        {
          free(*buffer);
          error = kEPUB3FileReadFromArchiveError;
        }
      }
    }
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
  
  if(unzGoToFirstFile(epub->archive) == UNZ_OK)
  {
    uint32_t stringLength = (uint32_t)strlen(requiredMimetype);
    if(unzOpenCurrentFile(epub->archive) == UNZ_OK)
    {
      int byteCount = unzReadCurrentFile(epub->archive, buffer, stringLength);
      if(byteCount == stringLength)
      {
        if(strncmp(requiredMimetype, buffer, stringLength) == 0)
        {
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
  if(error == kEPUB3Success)
  {
    reader = xmlReaderForMemory(buffer, bufferSize, "", NULL, XML_PARSE_RECOVER);
    if(reader != NULL)
    {
      int retVal;
      while((retVal = xmlTextReaderRead(reader)) == 1)
      {
        const char *rootFileName = "rootfile";
        const xmlChar *name = xmlTextReaderConstName(reader);

        if(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT &&
           strcmp((char *)name, rootFileName) == 0)
        {
          xmlChar *fullPath = xmlTextReaderGetAttribute(reader, BAD_CAST "full-path");
          if(fullPath != NULL)
          {
            // TODD: validate that the full-path attribute is of the form path-rootless
            //       see http://idpf.org/epub/30/spec/epub30-ocf.html#sec-container-metainf-container.xml
            foundPath = kEPUB3_YES;
            *rootPath = (char *)fullPath;
          }
          else
          {
            // The spec requires the full-path attribute
            error = kEPUB3XMLXDocumentInvalidError;
          }
          break;
        }
      }
      if(retVal < 0)
      {
        error = kEPUB3XMLParseError;
      }
      if(!foundPath)
      {
        error = kEPUB3XMLXElementNotFoundError;
      }
    }
    else
    {
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
  if(unzLocateFile(epub->archive, filename, 1) == UNZ_OK)
  {
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
  if(error == kEPUB3Success)
  {
    unz_file_info fileInfo;
    if(unzGetCurrentFileInfo(epub->archive, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
    {
      *uncompressedSize = (uint32_t)fileInfo.uncompressed_size;
      error = kEPUB3Success;
    }
  }
  return error;
}

u_long _GetFileCountInZipFile(unzFile file)
{
  unz_global_info gi;
	int err = unzGetGlobalInfo(file, &gi);
	if (err != UNZ_OK)
    return err;
	
	return gi.number_entry;
}
