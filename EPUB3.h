#include <stdio.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>

#ifndef EPUB3_h
#define EPUB3_h

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct EPUB3 * EPUB3Ref;
typedef struct EPUB3Metadata * EPUB3MetadataRef;

EPUB3Ref EPUB3Retain(EPUB3Ref epub);
void EPUB3Release(EPUB3Ref epub);
EPUB3MetadataRef EPUB3MetadataRetain(EPUB3MetadataRef metadata);
void EPUB3MetadataRelease(EPUB3MetadataRef metadata);

EPUB3MetadataRef EPUB3MetadataCreate();
EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub);
void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title);
void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata);
EPUB3Ref EPUB3Create();

#if defined(__cplusplus)
} //EXTERN "C"
#endif

#endif
