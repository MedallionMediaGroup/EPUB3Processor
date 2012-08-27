//
//  validators.h
//  EPUB3Processor
//
//  Created by Brian Buck on 6/25/12.
//  Copyright (c) 2012 Medallion Press, Inc. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify
//  it under the terms of the MIT license. See LICENSE-MIT for details.
//

#ifndef EPUB3Processor_validators_h
#define EPUB3Processor_validators_h

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <sys/types.h>

struct metaparse
{
    uint rtnVal;
    xmlChar *parseVal;
    xmlChar *arrVal[16];
} _mparse;

struct FilesInZipCount
{
    char** arrOfFiles;
    size_t fileCount;
};

// API implementation

const char* getMainJsonObjectAsString();

struct FilesInZipCount
listFilesInEpub(const char* epubFilePath);

uint validateOCF3(const char* epubFilePath,
                  const char* fileSystemRootDir,
                  int writeToDisk);

void cleanUpMemory();

void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output);

#endif
