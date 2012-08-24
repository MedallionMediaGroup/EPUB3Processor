#include <stdio.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include "unzip.h"
#include "EPUB3.h"

#ifndef EPUB3_private_h
#define EPUB3_private_h

#define EXPORT __attribute__((visibility("default")))

typedef enum { kEPUB3_NO = 0 , kEPUB3_YES = 1 } EPUB3Bool;

const char * kEPUB3TypeID;
const char * kEPUB3MetadataTypeID;
const char * kEPUB3ManifestTypeID;
const char * kEPUB3ManifestItemTypeID;
const char * kEPUB3SpineTypeID;
const char * kEPUB3SpineItemTypeID;


#pragma mark - Internal XML Parsing State

typedef enum {
  kEPUB3OPFStateRoot = 0,
  kEPUB3OPFStateMetadata,
  kEPUB3OPFStateManifest,
  kEPUB3OPFStateSpine
} EPUB3OPFParseState;

typedef struct _EPUB3OPFParseContext {
  EPUB3OPFParseState state;
  const xmlChar *tagName;
  int32_t attributeCount;
  char ** attributes;
  EPUB3Bool shouldParseTextNode;
} EPUB3OPFParseContext;

typedef EPUB3OPFParseContext * EPUB3OPFParseContextPtr;

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
  EPUB3ManifestRef manifest;
  EPUB3SpineRef spine;
  char * archivePath;
  unzFile archive;
  uint32_t archiveFileCount;
};

struct EPUB3Metadata {
  EPUB3Type _type;
  EPUB3Version version;
  char * title;
  char * _uniqueIdentifierID;
  char * identifier;
  char * language;
  // char * modified;
};

struct EPUB3ManifestItem {
  EPUB3Type _type;
  char * itemId;
  char * href;
  char * mediaType;
  char * properties;
};

#define MANIFEST_HASH_SIZE 128

typedef struct EPUB3ManifestItemListItem {
  EPUB3ManifestItemRef item;
  struct EPUB3ManifestItemListItem * next;
} * EPUB3ManifestItemListItemPtr;

struct EPUB3Manifest {
  EPUB3Type _type;
  EPUB3ManifestItemListItemPtr itemTable[MANIFEST_HASH_SIZE];
  int32_t itemCount;
};

typedef struct EPUB3SpineItemListItem {
  EPUB3SpineItemRef item;
  struct EPUB3SpineItemListItem * prev;
  struct EPUB3SpineItemListItem * next;
} * EPUB3SpineItemListItemPtr;

struct EPUB3Spine {
  EPUB3Type _type;
  int32_t itemCount;
  EPUB3SpineItemListItemPtr head;
  EPUB3SpineItemListItemPtr tail;
};

struct EPUB3SpineItem {
  EPUB3Type _type;
  EPUB3Bool isLinear;
  char * idref;
  EPUB3ManifestItemRef manifestItem; //weak ref
};

#pragma mark - Base Object

void _EPUB3ObjectRelease(void *object);
void _EPUB3ObjectRetain(void *object);
void * _EPUB3ObjectInitWithTypeID(void *object, const char *typeID);

#pragma mark - Main EPUB3 Object

EPUB3Ref EPUB3Create();
void EPUB3SetStringValue(char ** location, const char *value);
char * EPUB3CopyStringValue(char ** location);
void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata);
void EPUB3SetManifest(EPUB3Ref epub, EPUB3ManifestRef manifest);
void EPUB3SetSpine(EPUB3Ref epub, EPUB3SpineRef spine);

#pragma mark - Metadata

EPUB3MetadataRef EPUB3MetadataCreate();
void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title);
void EPUB3MetadataSetIdentifier(EPUB3MetadataRef metadata, const char * identifier);
void EPUB3MetadataSetLanguage(EPUB3MetadataRef metadata, const char * language);

#pragma mark - Manifest

EPUB3ManifestRef EPUB3ManifestCreate();
EPUB3ManifestItemRef EPUB3ManifestItemCreate();
void EPUB3ManifestInsertItem(EPUB3ManifestRef manifest, EPUB3ManifestItemRef item);
EPUB3ManifestItemRef EPUB3ManifestCopyItemWithId(EPUB3ManifestRef manifest, const char * itemId);
EPUB3ManifestItemListItemPtr _EPUB3ManifestFindItemWithId(EPUB3ManifestRef manifest, const char * itemId);

#pragma mark - Spine

EPUB3SpineRef EPUB3SpineCreate();
EPUB3SpineItemRef EPUB3SpineItemCreate();
void EPUB3SpineAppendItem(EPUB3SpineRef spine, EPUB3SpineItemRef item);
void EPUB3SpineItemSetManifestItem(EPUB3SpineItemRef spineItem, EPUB3ManifestItemRef manifestItem);

#pragma mark - XML Parsing

EPUB3Error EPUB3InitFromOPF(EPUB3Ref epub, const char * opfFilename);
void _EPUB3SaveParseContext(EPUB3OPFParseContextPtr *ctxPtr, EPUB3OPFParseState state, const xmlChar * tagName, int32_t attrCount, char ** attrs, EPUB3Bool shouldParseTextNode);
void _EPUB3PopAndFreeParseContext(EPUB3OPFParseContextPtr *contextPtr);
EPUB3Error _EPUB3ProcessXMLReaderNodeForMetadataInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context);
EPUB3Error _EPUB3ProcessXMLReaderNodeForManifestInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context);
EPUB3Error _EPUB3ProcessXMLReaderNodeForSpineInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context);
EPUB3Error _EPUB3ParseXMLReaderNodeForOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *currentContext);
EPUB3Error _EPUB3ParseFromOPFData(EPUB3Ref epub, void * buffer, uint32_t bufferSize);

#pragma mark - Validation

EPUB3Error EPUB3ValidateMimetype(EPUB3Ref epub);
EPUB3Error EPUB3ValidateFileExistsAndSeekInArchive(EPUB3Ref epub, const char * filename);

#pragma mark - File and Zip Functions

EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath);
EPUB3Error EPUB3CopyFileIntoBuffer(EPUB3Ref epub, void **buffer, uint32_t *bufferSize, uint32_t *bytesCopied, const char * filename);
uint32_t _GetFileCountInZipFile(unzFile file);
EPUB3Error EPUB3GetUncompressedSizeOfFileInArchive(EPUB3Ref epub, uint32_t *uncompressedSize, const char *filename);


#pragma mark - Hash function
// via: http://www.azillionmonkeys.com/qed/hash.html
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static inline uint32_t SuperFastHash(const char * data, int len)
{
  uint32_t hash = len, tmp;
  int rem;

  if (len <= 0 || data == NULL) return 0;

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits (data);
    tmp    = (get16bits (data+2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2*sizeof (uint16_t);
    hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3: hash += get16bits (data);
      hash ^= hash << 16;
      hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
      hash += hash >> 11;
      break;
    case 2: hash += get16bits (data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    case 1: hash += (signed char)*data;
      hash ^= hash << 10;
      hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

#endif
