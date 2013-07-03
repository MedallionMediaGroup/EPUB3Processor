//
//  EPUB3Processor.h
//  EPUB3Processor
//
//  Created by Brian Buck on 6/25/12.
//  Copyright (c) 2012 Medallion Press, Inc. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify
//  it under the terms of the MIT license. See LICENSE-MIT for details.
//

#ifndef EPUB3Processor_EPUB3Processor_h
#define EPUB3Processor_EPUB3Processor_h

#include "validators.h"

// EPUB VALIDATION & PARSE ERRORS
#define NO_ERRORS                   UINT32_C(0)
#define UNKNOWN_ZIP_ERR             UINT32_C(1)
#define ZIP_ERR                     UINT32_C(2)
#define NO_EPUB_AT_PATH             UINT32_C(3)
#define NO_FILES_IN_EPUB            UINT32_C(4)
#define MIMETYPE_NOT_FIRST          UINT32_C(5)
#define READ_BYTES_ERR              UINT32_C(6)
#define INVALID_EPUB_MIMETYPE       UINT32_C(7)
#define NO_FILE_BY_NAME             UINT32_C(8)
#define NO_CONTAINER_XML            UINT32_C(9)
#define XML_READ_MEM_ERR            UINT32_C(10)
#define XML_BUFFER_ZERO             UINT32_C(11)
#define XPATH_CONTEXT_ERR           UINT32_C(12)
#define XPATH_CONTEXT_NS_ERR        UINT32_C(13)
#define NULL_XPATH_PTR              UINT32_C(14)
#define NO_REFERENCED_OPF_FILE      UINT32_C(15)
#define OPF_PARSE_ERR               UINT32_C(16)
#define NO_FULLPATH_ATTR_ROOTFILE   UINT32_C(17)
#define OPF_MISSING_PACKAGE_DATA    UINT32_C(18)
#define METADATA_NO_IDENTIFIER      UINT32_C(19)
#define METADATA_NO_TITLE           UINT32_C(20)
#define METADATA_NO_LANG            UINT32_C(21)
#define METADATA_NO_CREATOR         UINT32_C(22)
#define METADATA_NO_PUB_DATE        UINT32_C(23)
#define METADATA_NO_COVER_IMG       UINT32_C(24)
#define SPINE_PROCESSING_ERR        UINT32_C(25)
#define FILE_IO_PATH_ARRAY_ERR      UINT32_C(26)
#define FILE_IO_ERR                 UINT32_C(27)
#define GET_ZIP_FILE_INFO_ERR       UINT32_C(28)
#define NOT_ALL_FILES_WRITTEN_ERR   UINT32_C(29)

#endif
