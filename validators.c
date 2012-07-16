//
//  validators.c
//  EPUB3Processor
//
//  Created by Brian Buck on 6/25/12.
//  Copyright (c) 2012 Medallion Press, Inc. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify
//  it under the terms of the MIT license. See LICENSE-MIT for details.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>

#include "EPUB3Processor.h"
#include "unzip.h"
// json-c library
#include "json.h"

#pragma mark -
#pragma mark function prototypes

unzFile
unzipFile(const char* filenamePath);
int
numberOfFilesInZip(unzFile unzipFile);
int
gotoFirstFile(unzFile unzipFile);
////struct FilesInZipContainerInfo
////getCurrentFileInZipInfo(unzFile unzipFile);
char*
getCurrentFileInZipInfoStr(unzFile unzipFile);
int
gotoNextFileInZip(unzFile unzipFile);
uint readFileFromEPubMatchingStr(unzFile unzipFile, char* matchString, uint bufferSize);
int readFileFromEpub(unzFile unzipFile, void* byteBuf, uint bufferSize);
uint locateFileInEpub(unzFile unzipFile, struct FilesInZipCount *filesInZip, char* fileToLocate);
uint checkMimetype(struct FilesInZipCount* fileInZip, unzFile unzipFile);
uint parse_opf_file(unzFile unzipFile, void** opfDataBuf, uint opfDataBufLength);
void parse_opf_package(xmlXPathContextPtr xpathCtx, xmlDocPtr doc);
void parse_metadata(xmlXPathContextPtr xpathCtx, xmlDocPtr doc, xmlChar* xpathExp);
//uint parse_all_metadata(void** opfDataBuf, uint opfDataBufLength);
uint parse_all_metadata(xmlDocPtr doc, xmlXPathContextPtr xpathCtx);
void parse_guide(xmlXPathContextPtr xpathCtx, xmlDocPtr doc);
void parse_bookcover(xmlXPathContextPtr xpathCtx, xmlDocPtr doc);
uint parse_spine(xmlDocPtr doc, xmlXPathContextPtr xpathCtx);
int writeContentDocumentsToDisk(unzFile unzipFile, const char *file_system_root_dir,
                                const char *OPF_root_dir, int manifest_array_length,
                                struct FilesInZipCount *filesAndCount);
void freeFilesAndCounts(struct FilesInZipCount *filesAndCount);
void cleanUp();

#pragma mark -
#pragma mark json objects

// array of files contained in EPUB
static int fileCount;
static char** listArray;
// store the package/EPUB version
static char* packageVersion;
// JSON structures
static json_object *main_json_object;
static json_object *top_level_array_json_object;
static json_object *metadata_json_object;
static json_object *manifest_json_object;
static json_object *spine_json_object;

#pragma mark -
#pragma mark utility functions

unzFile unzipFile(const char* filenamePath)
{
    unzFile _unzFile = unzOpen(filenamePath);
    return _unzFile;
}

// number of files in zip
// opensource: pure C implementation
int numberOfFilesInZip(unzFile unzipFile)
{
    unz_global_info gi;
	int err = unzGetGlobalInfo(unzipFile, &gi);
	if (err != UNZ_OK)
        return err;
	
	return gi.number_entry;
}

// goto first file in zip
// opensource: pure C implementation
int gotoFirstFile(unzFile unzipFile)
{
    return unzGoToFirstFile(unzipFile);
}

// contruct a FilesInZipContainerInfo for current file in zip
// opensource: pure C implementation
/*
struct FilesInZipContainerInfo
getCurrentFileInZipInfo(unzFile unzipFile)
{
    struct FilesInZipContainerInfo infos;
    char filename_inzip[256];
	unz_file_info file_info;
    
    int err= unzGetCurrentFileInfo(unzipFile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    
	if (err == UNZ_OK) {
        infos.fileNameInZip = filename_inzip;
        infos.file_info = file_info;
	}
    
    return infos;
}
*/

// opensource: pure C implementation
char*
getCurrentFileInZipInfoStr(unzFile unzipFile)
{
    char filename_inzip[256];
	unz_file_info file_info;
    
    int err = unzGetCurrentFileInfo(unzipFile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    
    if (err == UNZ_OK)
    {
        char *temp = (char*)malloc(sizeof(strlen(filename_inzip))+1);
        strcpy(temp, filename_inzip);
        if (temp != NULL)
            return temp;
    }
    
    return (char*)NULL;
    
}

// opensource: pure C implementation
int gotoNextFileInZip(unzFile unzipFile)
{
    return unzGoToNextFile(unzipFile);
}


uint readFileFromEPubMatchingStr(unzFile unzipFile, char* matchString, uint bufferSize)
{
    uint rtnCode = NO_ERRORS;
    void* byteBuf = (void*)malloc(sizeof(char)*bufferSize);
    
    int err= unzOpenCurrentFilePassword(unzipFile, NULL);
    if (err != UNZ_OK)
        rtnCode = READ_BYTES_ERR;
    else
    {
        err = unzReadCurrentFile(unzipFile, byteBuf, bufferSize);
        
        if (err < 0)
            rtnCode = READ_BYTES_ERR;
#ifdef DEBUG
        fprintf(stdout, "str cmp: %s : %s\n", (char*)byteBuf, matchString);  
#endif
        if (rtnCode == NO_ERRORS && strcmp((const char*)byteBuf, matchString) != 0)
            rtnCode = INVALID_EPUB_MIMETYPE;
    }
    
    free(byteBuf);
    return rtnCode;
}

int readFileFromEpub(unzFile unzipFile, void* byteBuf, uint bufferSize)
{
    if (byteBuf == NULL)
        return -1;
    
    int err = unzOpenCurrentFilePassword(unzipFile, NULL);
    if (err != UNZ_OK)
        err = -1;
    else
        err = unzReadCurrentFile(unzipFile, byteBuf, bufferSize);
    
    return err;
}

uint locateFileInEpub(unzFile unzipFile, struct FilesInZipCount *filesInZip, char* fileToLocate)
{
    uint rtnCode = NO_FILE_BY_NAME;
    
    for(int i=0; i < filesInZip->fileCount; i++)
    {
        if(strcmp(filesInZip->arrOfFiles[i], fileToLocate) == 0)
        {
            rtnCode = unzLocateFile(unzipFile, fileToLocate, 1);
#ifdef DEBUG
            fprintf(stdout, "return from unzLocateFile: %d\n", (int)rtnCode);      
#endif
            break;
        }
    }
    
    return rtnCode; // UNZ_OK or UNZ_END_OF_LIST_OF_FILE
}

void freeFilesAndCounts(struct FilesInZipCount *filesAndCount)
{
    // free memory
    for(int i=(filesAndCount->fileCount)-1; i >= 0; i--)
    {
        free(filesAndCount->arrOfFiles[i]);
    }
    free(filesAndCount->arrOfFiles); 
}

void cleanUp()
{    
    xmlCleanupParser();
    xmlCleanupMemory();
}

#pragma mark -
#pragma mark validators

uint checkMimetype(struct FilesInZipCount* fileInZip, unzFile unzipFile)
{
    /*
     * OCF 3 SPEC REF.
     *
     * check for first file being "mimetype" with contents = "application/epub+zip" (required)
     */
    
    if (fileInZip->fileCount > 0)
    {
        // mimetype first file in EPUB
        if (strcmp(fileInZip->arrOfFiles[0], "mimetype") != 0)
        {
            fprintf(stdout, "Error: EPUB mimetype not first file.\n");
            return MIMETYPE_NOT_FIRST;
        }
        
        // go to first file in EPUB should be mimetype
        if (gotoFirstFile(unzipFile) < 0)
        {
            fprintf(stdout, "Error: EPUB zip error. Unable to go to first file.\n");
            return UNKNOWN_ZIP_ERR;
        }
        // read the mimetype
        if (readFileFromEPubMatchingStr(unzipFile, "application/epub+zip", 256) > 0)
        {
            fprintf(stdout, "EPUB invalid mimetype.\n");
            return INVALID_EPUB_MIMETYPE;
        }
    }
    
    return NO_ERRORS;
}

// entry point for parsing entire opf file
uint parse_opf_file(unzFile unzipFile, void** opfDataBuf, uint opfDataBufLength)
{
#ifdef DEBUG
    fprintf(stdout, "OPF CONTENTS: %s\n", (char*)*opfDataBuf);              
#endif
 
    // create an xml doc from opf file
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx; 
    // Load XML document
    xmlInitParser();
    doc = xmlReadMemory(*opfDataBuf, opfDataBufLength, "", NULL, XML_PARSE_RECOVER);
    if (doc == NULL)
    {
        xmlFreeDoc(doc);
        return XML_READ_MEM_ERR;
    }
    
    // Create xpath evaluation context
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        xmlFreeDoc(doc);
        return XPATH_CONTEXT_ERR;
    }
    
    /** parse the meta data **/
    if (parse_all_metadata(doc, xpathCtx) != NO_ERRORS)
    {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return OPF_PARSE_ERR;
    }
    
    /*** parse the manifest ***/
    char *opfNSPrefix = "opf";
    char *opfNSHref = "http://www.idpf.org/2007/opf";
    
    if(xmlXPathRegisterNs(xpathCtx, (xmlChar*)opfNSPrefix, (xmlChar*)opfNSHref) != 0) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return XPATH_CONTEXT_NS_ERR;
    }
     
    xmlChar *manifestItemHrefXPathExpr = (xmlChar*)"//opf:manifest/opf:item";
    
    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEvalExpression(manifestItemHrefXPathExpr,
                                      xpathCtx);
    
    if(xpathObj == NULL) {
        xmlXPathFreeContext(xpathCtx); 
        xmlFreeDoc(doc);
        return NULL_XPATH_PTR;
    }
    
#ifdef DEBUG
    print_xpath_nodes(xpathObj->nodesetval, stdout);
#endif
    
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    if (size > 0) // construct the top level manifest object (array)
        manifest_json_object = json_object_new_array();
    
    for(int i = 0; i < size; ++i) {
        if(xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
            
            xmlNodePtr currentNode = xpathObj->nodesetval->nodeTab[i];
            
            // special check for 3.0 version cover-image as cover-image is part of the manifest
            // Ex. <item id="cover-image" properties="cover-image" href="images/9780316000000.jpg" media-type="image/jpeg"/>
            if (strcmp(packageVersion, "3.0") == 0)
            {
                xmlChar* _propVal = xmlGetProp(currentNode, (xmlChar*)"properties");
                if ( _propVal != NULL && strcmp((char*)_propVal, (char*)"cover-image") == 0)
                {
                    xmlFree(_propVal);
                    // found the cover image, now get the href and add it to the metadata
                    xmlChar* _hrefVal = xmlGetProp(currentNode, (xmlChar*)"href");
                    
                    json_object *_m_cover_object = json_object_new_object();
                    json_object_object_add(_m_cover_object, "coverImage", json_object_new_string((char*)_hrefVal));
                    json_object_array_add(metadata_json_object, _m_cover_object);
                    
                    _m_cover_object = json_object_new_object();
                    json_object_object_add(_m_cover_object, "coverImageRefType", json_object_new_string("href"));
                    json_object_array_add(metadata_json_object, _m_cover_object);
                    
                    xmlFree(_hrefVal);
                }
            } // end special check for EPUB 3 cover-image
            
            // id
            xmlChar* idAttrVal = xmlGetProp(currentNode, (xmlChar*)"id");
            
            // href
            xmlChar* hrefAttrVal = xmlGetProp(currentNode, (xmlChar*)"href");
            
            // media-type
            xmlChar* mediaTypeAttrVal = xmlGetProp(currentNode, (xmlChar*)"media-type");
            
            if (idAttrVal == NULL || hrefAttrVal == NULL || mediaTypeAttrVal == NULL)
            {
                xmlFree(idAttrVal);
                xmlFree(hrefAttrVal);
                xmlFree(mediaTypeAttrVal);
                xmlXPathFreeContext(xpathCtx); 
                xmlFreeDoc(doc);
                return OPF_PARSE_ERR;
            }
            else
            {
                // create a manifest json data structure separate from the metadata array.
                // the manifest is an array of k/v pairs where key = item id and value is
                // an array containing (in order) content document path, and second element
                // content document media type (ex. application/xhtml+xml)
                
                // element k/v
                json_object *_manifest_item = json_object_new_object();
                
                // values
                json_object *_manifest_item_array = json_object_new_array();
                json_object_array_add(_manifest_item_array, json_object_new_string((char*)hrefAttrVal));
                json_object_array_add(_manifest_item_array, json_object_new_string((char*)mediaTypeAttrVal));
                // add the item k/v
                json_object_object_add(_manifest_item, (const char*)idAttrVal, _manifest_item_array);
                
                // add it to the top level manifest array
                json_object_array_add(manifest_json_object, _manifest_item);
            }
            
            xmlFree(idAttrVal);
            xmlFree(hrefAttrVal);
            xmlFree(mediaTypeAttrVal);
            
        } // closing for XML_ELEMENT_NODE check (note that sibling elements in the manifest should be XML_ELEMENT_NODE
    } // closing for loop iteration over manifest items
    
    // add manifest_json_object to the main_json_object if it contains objects (it should)
    if (json_object_array_length(manifest_json_object) > 0)
        json_object_array_add(top_level_array_json_object, manifest_json_object);
    
    /*** parse the spine ***/
    
    if (parse_spine(doc, xpathCtx) != NO_ERRORS)
    {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return SPINE_PROCESSING_ERR;
    }
    
    xmlFreeDoc(doc);
    return NO_ERRORS;
}

void parse_opf_package(xmlXPathContextPtr xpathCtx, xmlDocPtr doc)
{
    int arrValIdx = 0;
    xmlXPathObjectPtr xpathObj;
    xmlChar xpathExp[13];
    sprintf((char*)&xpathExp, "/opf:package");
    xpathObj = xmlXPathEvalExpression(xpathExp, xpathCtx);
    
    if(xpathObj == NULL) {
        xmlXPathFreeContext(xpathCtx); 
        xmlFreeDoc(doc);
        return;
    }
    
    // Get the value
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    if (size == 0)
    {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return;
    }
    
    if(xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
        // required attributes an epub 3.0 package element
        // version = 3.0 (for this spec)
        // unique-identifier
        // profile = "http://www.idpf.org/epub/30/profile/package/"
        // xml:lang = ex "en" or "en-US"
        
        // version
        xmlChar* versionAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"version");
        if (versionAttrVal == NULL)
        {
            xmlFree(versionAttrVal);
            xmlXPathFreeContext(xpathCtx); 
            xmlFreeDoc(doc);            
            return;   
        }
        
#if DEBUG                                
        fprintf(stdout, "PACKAGE VERSION: %s\n", (const char*)versionAttrVal);
#endif
        _mparse.arrVal[arrValIdx++] = versionAttrVal;
        
        // unique-identifier
        xmlChar* uidAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"unique-identifier");
        if (uidAttrVal == NULL)
        {
            xmlFree(uidAttrVal);
            xmlXPathFreeContext(xpathCtx); 
            xmlFreeDoc(doc);            
            return;   
        }
        
#if DEBUG                                
        fprintf(stdout, "PACKAGE UID: %s\n", (const char*)uidAttrVal);
#endif
        _mparse.arrVal[arrValIdx++] = uidAttrVal;
        
        // profile
        char _ver[4];
        sprintf((char*)&_ver, "3.0");
        int _rtnStrCmp = strcmp((const char*)_mparse.arrVal[0], (const char*)_ver);
        if (_rtnStrCmp == 0)
        {
            xmlChar* profileAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"profile");
            if (profileAttrVal == NULL)
            {
                xmlFree(profileAttrVal);
                xmlXPathFreeContext(xpathCtx); 
                xmlFreeDoc(doc);            
                return;   
            }
#if DEBUG                                
            fprintf(stdout, "PACKAGE PROFILE: %s\n", (const char*)profileAttrVal);
#endif
            _mparse.arrVal[arrValIdx++] = profileAttrVal;
            
        }
        
        // xml:lang
        if (_rtnStrCmp == 0)
        {
            xmlChar* langAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"lang");
            if (langAttrVal == NULL)
            {
                xmlFree(langAttrVal);
                xmlXPathFreeContext(xpathCtx); 
                xmlFreeDoc(doc);            
                return;   
            }
            
#if DEBUG                                
            fprintf(stdout, "PACKAGE XML:LANG: %s\n", (const char*)langAttrVal);
#endif
            _mparse.arrVal[arrValIdx++] = langAttrVal;
        }
        
        // end required <package/> attributes
        // TODO : need impl for backward compatibility to ePub 2.0
        
        _mparse.arrVal[arrValIdx] = '\0';
        _mparse.rtnVal = NO_ERRORS;
    }
    
    
}

void parse_metadata(xmlXPathContextPtr xpathCtx, xmlDocPtr doc, xmlChar* xpathExp)
{
    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEvalExpression(xpathExp, xpathCtx);
    
    if(xpathObj == NULL || xmlXPathNodeSetIsEmpty(xpathObj->nodesetval)) {
        _mparse.rtnVal = NULL_XPATH_PTR;
        _mparse.parseVal = NULL;
        return;
    }
    
    // Get the value
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    if (size == 0)
    {
        _mparse.rtnVal = NULL_XPATH_PTR;
        _mparse.parseVal = NULL;
        return;
    }
    
    if(xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
        // contents
        // TODO: fix this hack for 2.0 cover
        xmlChar* _contents;
        _contents = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
        
        if (_contents == NULL)
        {
            xmlFree(_contents);
            _mparse.rtnVal = NULL_XPATH_PTR;
            _mparse.parseVal = NULL;
            return;
        }
        else
        {
#if DEBUG
            fprintf(stdout, "metadata content: %s\n", _contents);
#endif
            _mparse.rtnVal = NO_ERRORS;
            _mparse.parseVal = _contents;
        }
    }
}

uint parse_all_metadata(xmlDocPtr doc, xmlXPathContextPtr xpathCtx) //(void** opfDataBuf, uint opfDataBufLength)
{
    //xmlDocPtr doc;
    //xmlXPathContextPtr xpathCtx; 
    // Load XML document
    //xmlInitParser();
    //doc = xmlReadMemory(*opfDataBuf, opfDataBufLength, "", NULL, XML_PARSE_RECOVER);
    //if (doc == NULL)
    //{
    //    xmlFreeDoc(doc);
    //    return XML_READ_MEM_ERR;
    //}
    
    // Create xpath evaluation context
    //xpathCtx = xmlXPathNewContext(doc);
    //if(xpathCtx == NULL)
    //{
    //    xmlFreeDoc(doc);
    //    return XPATH_CONTEXT_ERR;
    //}
    
    // Top level
    xmlChar* opfNSPrefix = (xmlChar*)"opf";
    xmlChar* opfNSHref = (xmlChar*)"http://www.idpf.org/2007/opf";
    
    if(xmlXPathRegisterNs(xpathCtx, opfNSPrefix, opfNSHref) != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return XPATH_CONTEXT_NS_ERR;
    }
    
    // metadata dc
    opfNSPrefix = (xmlChar*)"dc";
    opfNSHref = (xmlChar*)"http://purl.org/dc/elements/1.1/";
    
    if(xmlXPathRegisterNs(xpathCtx, opfNSPrefix, opfNSHref) != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return XPATH_CONTEXT_NS_ERR;
    }
    
    /*** get the dc_identifier ***/
    _mparse.rtnVal = METADATA_NO_IDENTIFIER;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    xmlChar dcIdentifierXPathExpr[29];
    sprintf((char*)&dcIdentifierXPathExpr, "//opf:metadata/dc:identifier");
    //
    parse_metadata(xpathCtx, doc, dcIdentifierXPathExpr);
    
    if ( _mparse.rtnVal  != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return OPF_PARSE_ERR;
    }
    else
    {
        main_json_object = json_object_new_object();
        top_level_array_json_object = json_object_new_array();
        metadata_json_object = json_object_new_array();
        json_object *_m_dcIdentifier = json_object_new_object();
        
        // build the metadata k/v structures
        json_object_object_add(_m_dcIdentifier, "dcIdentifier", json_object_new_string((char*)_mparse.parseVal));
        json_object_array_add(metadata_json_object, _m_dcIdentifier);
        json_object_array_add(top_level_array_json_object, metadata_json_object);
        json_object_object_add(main_json_object, (char*)_mparse.parseVal, top_level_array_json_object);
        
#ifdef DEBUG
        fprintf(stdout,"main_json_object=\n");
        json_object_object_foreach(main_json_object, key, val) {
            fprintf(stdout, "\t%s: %s\n", key, json_object_to_json_string(val));
        }
        fprintf(stdout,"main_json_object.to_string()=%s\n", json_object_to_json_string(main_json_object));
#endif
        
        ////json_object_put(main_json_object);
    }
    
    /*** parse the <root> package element ***/

    _mparse.rtnVal = OPF_MISSING_PACKAGE_DATA;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    
    parse_opf_package(xpathCtx, doc);
    
    if ( _mparse.rtnVal  != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return OPF_MISSING_PACKAGE_DATA;
    }
    else
    {
        int len = sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal);
#if DEBUG
        for (int i=0; i < len; i++)
            fprintf(stdout, "PACKAGE ATTR VALS: %s\n", (unsigned char*)_mparse.arrVal[i]);
#endif
        for (int i=0; i < len; i++)
        {
            if (_mparse.arrVal[i] == NULL)
                continue;
            
            json_object *_m_package_object = json_object_new_object();
            
            // TODO: the is a hack as the array is ordered by:
            // (<version>, <uid>, <profile>, <lang>)
            char *_keyStr;
            switch (i) {
                case 0:
                    _keyStr = "packageVersion";
                    packageVersion = (char*)_mparse.arrVal[i];
                    break;
                case 1:
                    _keyStr = "packageUniqueIdentifier";
                    break;
                case 2:
                    _keyStr = "packageProfile";
                    break;
                case 3:
                    _keyStr = "packageLanguage";
                    break;
                default:
                    break;
            }
            json_object_object_add(_m_package_object, _keyStr, json_object_new_string((char*)_mparse.arrVal[i]));
            json_object_array_add(metadata_json_object, _m_package_object);
        }
        
#ifdef DEBUG
        fprintf(stdout, "main_json_object=\n");
        json_object_object_foreach(main_json_object, key, val) {
            fprintf(stdout, "\t%s: %s\n", key, json_object_to_json_string(val));
        }
        fprintf(stdout, "main_json_object.to_string()=%s\n", json_object_to_json_string(main_json_object));
#endif
    }
    
    
    /*** get cover (2.0 only) ***/
    _mparse.rtnVal = METADATA_NO_COVER_IMG;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    
    if (strcmp(packageVersion, "2.0") == 0)
    {
        parse_guide(xpathCtx, doc);
        
        if ( _mparse.rtnVal == 0 )
        {
            json_object *_m_cover_object = json_object_new_object();
            json_object_object_add(_m_cover_object, "coverImage", json_object_new_string((char*)_mparse.parseVal));
            json_object_array_add(metadata_json_object, _m_cover_object);
            
            _m_cover_object = json_object_new_object();
            json_object_object_add(_m_cover_object, "coverImageRefType", json_object_new_string("idref"));
            json_object_array_add(metadata_json_object, _m_cover_object);
        }
    }
    
    /*** get title ***/
    _mparse.rtnVal = METADATA_NO_TITLE;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    xmlChar dcTitleXPathExpr[24];
    sprintf((char*)&dcTitleXPathExpr, "//opf:metadata/dc:title");
    //
    parse_metadata(xpathCtx, doc, dcTitleXPathExpr);
    
    if ( _mparse.rtnVal  != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return METADATA_NO_TITLE;
    }
    else
    {
        json_object *_m_title_object = json_object_new_object();
        json_object_object_add(_m_title_object, "title", json_object_new_string((char*)_mparse.parseVal));
        json_object_array_add(metadata_json_object, _m_title_object);
    }
    
    /*** get language ***/
    _mparse.rtnVal = METADATA_NO_LANG;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    xmlChar dcLanguageXPathExpr[27];
    sprintf((char*)&dcLanguageXPathExpr, "//opf:metadata/dc:language");
    //
    parse_metadata(xpathCtx, doc, dcLanguageXPathExpr);
    
    if ( _mparse.rtnVal  != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return METADATA_NO_LANG;
    }
    else
    {
        json_object *_m_language_object = json_object_new_object();
        json_object_object_add(_m_language_object, "language", json_object_new_string((char*)_mparse.parseVal));
        json_object_array_add(metadata_json_object, _m_language_object);
    }
    
    // ABOVE DC: ELEMENTS ARE REQUIRED, BUT THERE ARE SEVERAL ADDITIONAL <META ..
    // ELEMENTS THAT NEED TO BE PARSED.
    // SEE: http://idpf.org/epub/30/spec/epub30-publications.html#sec-package-content-conf
    
    // for now, let's get these values
    /* CREATOR/AUTHOR
     3.0 -> <meta property="dcterms:creator" id="auth">Herman Melville</meta>
     2.0 -> <dc:creator opf:file-as="Joyce, James">James Joyce</dc:creator>
     PUB DATE
     3.0 -> <meta property="dcterms:modified">2011-07-11T12:00:00Z</meta>
     2.0 -> <dc:date opf:event="publication">2003-07-01</dc:date>
     COVER (to use as a thumbnail in library)
     3.0 -> EX: this is a manifest/item entry properties="cover-image":
     <item id="cover-image" properties="cover-image" href="images/9780316000000.jpg" media-type="image/jpeg"/>
     2.0 -> <meta content="item16" name="cover"/>
     */
    
    /*** get creator ***/
    _mparse.rtnVal = METADATA_NO_CREATOR;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    
    if (strcmp(packageVersion, "3.0") == 0)
    {
        xmlChar dcCreatorXPathExpr[53];
        sprintf((char*)&dcCreatorXPathExpr, "//opf:metadata/opf:meta[@property=\'dcterms:creator\']");
        //
        parse_metadata(xpathCtx, doc, dcCreatorXPathExpr);        
    }
    else // "2.0"
    {
        xmlChar dcCreatorXPathExpr[26];
        sprintf((char*)&dcCreatorXPathExpr, "//opf:metadata/dc:creator");
        //
        parse_metadata(xpathCtx, doc, dcCreatorXPathExpr);        
    }
    
    if ( _mparse.rtnVal  != 0)
    {
        //xmlXPathFreeContext(xpathCtx);
        //xmlFreeDoc(doc);
        return METADATA_NO_CREATOR;
    }
    else
    {
        json_object *_m_creator_object = json_object_new_object();
        json_object_object_add(_m_creator_object, "creatorFullName", json_object_new_string((char*)_mparse.parseVal));
        json_object_array_add(metadata_json_object, _m_creator_object);
    }
    
    /*** get pub date ***/
    _mparse.rtnVal = METADATA_NO_PUB_DATE;
    _mparse.parseVal = NULL;
    memset(_mparse.arrVal, '\0', sizeof(_mparse.arrVal)/sizeof(*_mparse.arrVal));
    
    if (strcmp(packageVersion, "3.0") == 0)
    {
        xmlChar dcPubDateXPathExpr[54];
        sprintf((char*)&dcPubDateXPathExpr, "//opf:metadata/opf:meta[@property=\'dcterms:modified\']");
        //
        parse_metadata(xpathCtx, doc, dcPubDateXPathExpr);        
    }
    else // "2.0"
    {
        xmlChar dcPubDateXPathExpr[23];
        sprintf((char*)&dcPubDateXPathExpr, "//opf:metadata/dc:date");
        parse_metadata(xpathCtx, doc, dcPubDateXPathExpr);
        //xmlChar dcPubDateXPathExpr[51];
        //sprintf((char*)&dcPubDateXPathExpr, "//opf:metadata/dc:date[@opf:event=\'publication\']");
        //
        //parse_metadata(xpathCtx, doc, dcPubDateXPathExpr);
    }
    
    if ( _mparse.rtnVal  != 0)
    {
        // TODO: fix this parse code - stub out with 1900-01-01
        json_object *_m_pubdate_object = json_object_new_object();
        json_object_object_add(_m_pubdate_object, "publicationDate", json_object_new_string("1900-01-01"));
        json_object_array_add(metadata_json_object, _m_pubdate_object);
    }
    else
    {
        json_object *_m_pubdate_object = json_object_new_object();
        json_object_object_add(_m_pubdate_object, "publicationDate", json_object_new_string((char*)_mparse.parseVal));
        json_object_array_add(metadata_json_object, _m_pubdate_object);
    }
    
    //xmlFreeDoc(doc);
    return NO_ERRORS;
}


void parse_guide(xmlXPathContextPtr xpathCtx, xmlDocPtr doc)
{
    // Process the guide section (if exists)
    // Note: Per OPF 2.0.1 spec, <guide> is a top level node
    
    /* Example
     <guide>
     <reference href="cover.jpg" type="cover" title="Cover Image"/>
     <reference href="www.gutenberg.org@files@4300@4300-h@4300-h-0.htm#pgepubid00001" type="toc" title="Contents"/>
     </guide>
     */
   
    // Create xpath evaluation context
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    
    // Top level
    char* opfNSPrefix = "opf";
    char* opfNSHref = "http://www.idpf.org/2007/opf";
    
    if(xmlXPathRegisterNs(xpathCtx, (xmlChar*)opfNSPrefix, (xmlChar*)opfNSHref) != 0) {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    
    // Book cover
    xmlChar dcCoverImgXPathExpr[41];
    sprintf((char*)&dcCoverImgXPathExpr, "//opf:guide/opf:reference[@type='cover']");
    
    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEvalExpression(dcCoverImgXPathExpr, xpathCtx);
    
    // if failed try to parse the meta data for common location
    if(xpathObj == NULL) {        
        return parse_bookcover(xpathCtx, doc);
    }
    if(xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
    {
        return parse_bookcover(xpathCtx, doc);
    }
    
    // cover exists now get the href value
    if(xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
        xmlChar* _itemHrefRef = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"href");
        
#if DEBUG
        fprintf(stdout, "guide reference book cover href: %s\n", _itemHrefRef);
#endif
        _mparse.rtnVal = NO_ERRORS;
        _mparse.parseVal = _itemHrefRef;
    }
    
    // TODO : need to parse additional item if necessary
}


void parse_bookcover(xmlXPathContextPtr xpathCtx, xmlDocPtr doc)
{
    // Create xpath evaluation context
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    
    // Top level
    char* opfNSPrefix = "opf";
    char* opfNSHref = "http://www.idpf.org/2007/opf";
    
    if(xmlXPathRegisterNs(xpathCtx, (xmlChar*)opfNSPrefix, (xmlChar*)opfNSHref) != 0) {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    
    // create the xpath expression for the meta tag first, if not found,
    // need to look in the <guide> section of the manifest
    // typical example: <meta content="itemXX" name="cover"/>
    
    xmlChar *coverImgXPath =(xmlChar*)"//opf:metadata/opf:meta[@name='cover']";
    
    //xmlChar dcCoverImgXPathExpr[39];
    //sprintf((char*)&dcCoverImgXPathExpr, "//opf:metadata/opf:meta[@name='cover']");
    
    xmlXPathObjectPtr xpathCoverObj;
    xpathCoverObj = xmlXPathEvalExpression(coverImgXPath, xpathCtx);
    
    if(xpathCoverObj == NULL) {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    if(xmlXPathNodeSetIsEmpty(xpathCoverObj->nodesetval))
    {
        xmlXPathFreeContext(xpathCtx);
        return;
    }
    
    // found the node so continue to parse until item href value
    if(xpathCoverObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
        
        // "itemXX" id ref in manifest
        xmlChar* _contentItemRef = xmlGetProp(xpathCoverObj->nodesetval->nodeTab[0], (xmlChar*)"content");
#if DEBUG
        fprintf(stdout, "COVER PAGE ATTR: %s\n", (char*)_contentItemRef);
#endif
        int itemRefStrLen = strlen((char*)_contentItemRef);
        xmlChar dcCoverItemManifestXPathExpr[31+itemRefStrLen];
        sprintf((char*)&dcCoverItemManifestXPathExpr, "//opf:manifest/opf:item[@id='%s']", (char*)_contentItemRef);
        
        xmlXPathObjectPtr xpathObjForItem;
        xpathObjForItem = xmlXPathEvalExpression(dcCoverItemManifestXPathExpr, xpathCtx);
        
        if(xpathObjForItem == NULL) {
            xmlXPathFreeContext(xpathCtx); 
            return;
        }
        if(xmlXPathNodeSetIsEmpty(xpathObjForItem->nodesetval))
        {
            xmlXPathFreeContext(xpathCtx);
            return;
        }
        
        if(xpathObjForItem->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
            xmlChar* _itemHrefRef = xmlGetProp(xpathObjForItem->nodesetval->nodeTab[0], (xmlChar*)"href");
            
#if DEBUG
            fprintf(stdout, "manifest item book cover href: %s\n", _itemHrefRef);
#endif
            _mparse.rtnVal = NO_ERRORS;
            _mparse.parseVal = _itemHrefRef;
        }
    }
}

uint parse_spine(xmlDocPtr doc, xmlXPathContextPtr xpathCtx)
{   
    // Top level
    xmlChar* opfNSPrefix = (xmlChar*)"opf";
    xmlChar* opfNSHref = (xmlChar*)"http://www.idpf.org/2007/opf";
    
    if(xmlXPathRegisterNs(xpathCtx, opfNSPrefix, opfNSHref) != 0)
        return XPATH_CONTEXT_NS_ERR;
    
    xmlChar *spineItemRefXPathExpr = (xmlChar*)"//opf:spine/opf:itemref";
    
    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEvalExpression(spineItemRefXPathExpr,
                                      xpathCtx);
    if(xpathObj == NULL)
        return NULL_XPATH_PTR;
    
    // Get attributes of the spine.
    int foundLinearAttr = 0;
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    
    if (size > 0) // construct the top level spine object (array)
        spine_json_object = json_object_new_array();
    
    for(int i = 0; i < size; ++i) {
        if(xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
            
            foundLinearAttr = 0;
            
            // spine idref
            xmlChar* idRefAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[i],
                                               (xmlChar*)"idref");
            
            // spine linear flag
            xmlChar* linearAttrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[i],
                                                (xmlChar*)"linear");
            if (linearAttrVal != NULL)
                foundLinearAttr = 1;
            else
            {
                foundLinearAttr = 0;
                char _linear[4];
                sprintf((char*)&_linear, "yes");
                linearAttrVal = (xmlChar*)_linear;
            }
            
            if (idRefAttrVal == NULL)
            {
                xmlFree(idRefAttrVal);
                if (foundLinearAttr)
                    xmlFree(linearAttrVal);
                return SPINE_PROCESSING_ERR;
            }
            else
            {
                // create an array for the spine item
                json_object *_spine_item_array = json_object_new_array();
                json_object_array_add(_spine_item_array, json_object_new_string((char*)idRefAttrVal));
                json_object_array_add(_spine_item_array, json_object_new_string((char*)linearAttrVal));
                // add the main spine array
                json_object_array_add(spine_json_object, _spine_item_array);
            }
            
            xmlFree(idRefAttrVal);
            if (foundLinearAttr)
                xmlFree(linearAttrVal);
        } // closing for XML_ELEMENT_NODE check
    } // closing loop for iterating over spine elements
    
    // add spine_json_object to the main_json_object if it contains objects (it should)
    if (json_object_array_length(spine_json_object) > 0)
        json_object_array_add(top_level_array_json_object, spine_json_object);
    
    return NO_ERRORS;
}

int writeContentDocumentsToDisk(unzipFile, file_system_root_dir, OPF_root_dir, manifest_array_length, filesAndCount)
unzFile unzipFile;
const char *file_system_root_dir;
const char *OPF_root_dir;
int manifest_array_length;
struct FilesInZipCount *filesAndCount;

{
    int rtnVal = NO_ERRORS;
    // write content documents to disk
    // note: this needs testing on Win x86 x64 platforms (i.e non Unix based OS)
    
    // loop over the content documents and parse the path needed to create the directories
    // the content document path needs to append to a pre-defined location on the file system
    // ...
    
    // manifest_json_object => array of k/v pairs where k = item id and v = array of 2 elements
    // where element 0 = content document href and element 1 = media-type.
    
    char **path_array = (char**)malloc(sizeof(char*));
    
    for (int idx = 0; idx < manifest_array_length; idx++)
    {
        json_object *current_item = json_object_array_get_idx(manifest_json_object, idx);
        if (current_item != NULL)
        {
            // single entry per manifest: k = item ref; v = array (href,media-type)
            struct lh_entry *entry = json_object_get_object(current_item)->head;
            struct json_object *val = (struct json_object*)entry->v;
            
            json_object *item_href = json_object_array_get_idx(val, 0); // 0 = href; 1 = media-type
            char *href_str_val = (char*)json_object_get_string(item_href);
            
            char *token_href_val = (char*)malloc( strlen(href_str_val) + 1 );
            strcpy(token_href_val, href_str_val);
            ////strcpy(orig_href_val, href_str_val);
            
            
#ifdef DEBUG
            fprintf(stdout, "loop manifest for disk write: %s\n", href_str_val);                      
#endif
            
            // parse the content document path for any directories to create
            int loop_count = 0;
            int has_content = 0;
            ////char **path_array = (char**)malloc(sizeof(char*));
            
            char *token = strtok(token_href_val, "/");
            if (token != NULL)
                has_content = 1;
            while (token != NULL)
            {
                // add token to path array
                path_array[loop_count] = (char*)malloc(strlen(token)+1);
                
                if (path_array[loop_count] != NULL) {
                    strcpy(path_array[loop_count],token);
                    token = strtok(NULL, "/");
                    if (token != NULL) {
                        path_array = (char**)realloc(path_array,((++loop_count)+1)*sizeof(char*));
                    }
                }
                else {
                    // unable to allocate path_array element
                    // free array
                    for(int arrIdx = loop_count; arrIdx >= 0; arrIdx--)
                        free( path_array[arrIdx] );
                    free(path_array);
                    return FILE_IO_PATH_ARRAY_ERR;
                }
            }
            
            // check path_array element, create directories and write content document manifest file to disk
            if (has_content)
            {
                char *content_root_path = (OPF_root_dir != NULL) ?
                (char*)malloc(strlen(href_str_val) + strlen(OPF_root_dir) + 2) :
                (char*)malloc(strlen(href_str_val) + 1);
                if (OPF_root_dir != NULL)
                    sprintf(content_root_path, "%s/%s", OPF_root_dir, href_str_val);
                else
                    strcpy(content_root_path, href_str_val); 
                
                if (loop_count == 0) // single element so this IS the content document file.
                {
                    rtnVal = locateFileInEpub(unzipFile, filesAndCount, content_root_path);
                    if (rtnVal != NO_ERRORS)
                    {
                        freeFilesAndCounts(filesAndCount);
                        return rtnVal;
                    }
                    
                    // need to get the current file uncompressed size
                    char filename_inzip[256];
                    unz_file_info file_info;
                    int getinfoerr = unzGetCurrentFileInfo(unzipFile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
                    if (getinfoerr != UNZ_OK)
                    {
                        freeFilesAndCounts(filesAndCount);
                        return GET_ZIP_FILE_INFO_ERR;
                    }
                    
                    // read content document into mem buffer
                    uint bufferSize = file_info.uncompressed_size;
                    void* byteBuf = (void*)malloc(bufferSize);
                    rtnVal = readFileFromEpub(unzipFile, byteBuf, bufferSize);
                    if (rtnVal < 0)
                    {
                        freeFilesAndCounts(filesAndCount);
                        free(byteBuf);
                        return FILE_IO_ERR;
                    }
                    
                    int err=UNZ_OK;
                    FILE *fout=NULL;
                    
                    int pathbuffsize = (int)strlen(file_system_root_dir) + (int)(strlen(path_array[0])+1);
                    char *fullpathdir = (char*)malloc(pathbuffsize);
                    
                    sprintf(fullpathdir, "%s%s", file_system_root_dir, path_array[0]);
                    
                    fout = fopen(fullpathdir,"wb");
                    
                    if (fout==NULL)
                    {
                        printf("error opening %s\n",fullpathdir);
                    }
                    else
                    {
                        printf(" extracting: %s\n",fullpathdir);
                        
                        size_t objwrite = fwrite(byteBuf, bufferSize, 1, fout);
                        if (objwrite != 1)
                        {
                            printf("error in writing extracted file\n");
                            fclose(fout);
                            return FILE_IO_ERR;
                        }
                        
                        if (fout)
                            fclose(fout);
                        
                        unzCloseCurrentFile(unzipFile);
                    }
                    
                    free(byteBuf);
                    free(fullpathdir);
                    
                }
                else
                {                    
                    int pathbuffsize = (int)(strlen(file_system_root_dir)+1); 
                    char *fullpathdir = (char*)malloc((pathbuffsize * sizeof(char)));
                    sprintf(fullpathdir, "%s", file_system_root_dir);
                    // end hack
                    
                    for(int arrIdx = 0; arrIdx <= loop_count; arrIdx++)
                    {
                        if (arrIdx < loop_count) // directory
                        {
                            // +1 for path separator '/'
                            pathbuffsize = pathbuffsize + (int)(strlen(path_array[arrIdx])+1);
                            
                            fullpathdir = (char*)(realloc(fullpathdir, pathbuffsize));
                            sprintf(fullpathdir, "%s%s%s", fullpathdir, path_array[arrIdx], "/");
                            
                            int mkdirstatus = mkdir(fullpathdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                            
                            if( mkdirstatus == 0 || errno == EEXIST) // success or directory exists
                            {
                                fprintf(stdout, "mkdir: directory created or exists.\n"); 
                            }
                            else {
                                fprintf(stdout, "mkdir: failure errno: %d\n", errno);
                            }
                        }
                        else
                        {
                            rtnVal = locateFileInEpub(unzipFile, filesAndCount, content_root_path);
                            if (rtnVal != NO_ERRORS)
                            {
                                freeFilesAndCounts(filesAndCount);
                                return rtnVal;
                            }
                            
                            
                            // need to get the current file uncompressed size
                            char filename_inzip[256];
                            unz_file_info file_info;
                            int getinfoerr = unzGetCurrentFileInfo(unzipFile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
                            if (getinfoerr != UNZ_OK)
                            {
                                freeFilesAndCounts(filesAndCount);
                                return GET_ZIP_FILE_INFO_ERR;
                            }
                            
                            // read content document into mem buffer
                            uLong bufferSize = file_info.uncompressed_size;
                            void* byteBuf = (void*)malloc(bufferSize);
                            rtnVal = readFileFromEpub(unzipFile, byteBuf, bufferSize);
                            if (rtnVal < 0)
                            {
                                freeFilesAndCounts(filesAndCount);
                                free(byteBuf);
                                return FILE_IO_ERR;
                            }
                            
                            int err=UNZ_OK;
                            FILE *fout=NULL;
                            
                            pathbuffsize = pathbuffsize + (int)(strlen(path_array[arrIdx]));
                            fullpathdir = (char*)(realloc(fullpathdir, pathbuffsize));
                            sprintf(fullpathdir, "%s%s", fullpathdir, path_array[arrIdx]);
                            
                            fout = fopen(fullpathdir,"wb");
                            
                            if (fout==NULL)
                            {
                                printf("error opening %s\n",fullpathdir);
                            }
                            else
                            {
                                printf(" extracting: %s\n",fullpathdir);
                                
                                size_t objwrite = fwrite(byteBuf, bufferSize, 1, fout);
                                if (objwrite != 1)
                                {
                                    fclose(fout);
                                    return FILE_IO_ERR;
                                }
                                
                                if (fout)
                                    fclose(fout);
                                
                                unzCloseCurrentFile(unzipFile); /* don't lose the error */
                            }
                            
                            free(byteBuf);
                        }
                    } // end iterating through path tokens
                    
                    // free path builders
                    free(fullpathdir);
                    free(token_href_val);
                    free(content_root_path);
                }
            }
            
            // free array
            if (has_content) {
                for(int arrIdx = loop_count; arrIdx >= 0; arrIdx--)
                    free( path_array[arrIdx] );
            }
        }
        else {
            if (path_array != NULL)
                free(path_array);
            return NOT_ALL_FILES_WRITTEN_ERR;
        }
        
    } // closing manifest loop on content documents
    
    if (path_array != NULL)
        free(path_array);
    
    return NO_ERRORS;
}


#pragma mark -
#pragma mark API implementation

const char* getMainJsonObjectAsString()
{
    return json_object_to_json_string(main_json_object);
}

struct FilesInZipCount
listFilesInEpub(const char* epubFilePath)
{
    size_t fileCountInEpub;
    struct FilesInZipCount filesAndCount;
    
    // get the unzipped file
    unzFile _unzFile = unzipFile(epubFilePath);
    if (_unzFile == NULL) // punt
        return filesAndCount;
    
    // get the number of entries
    fileCount = numberOfFilesInZip(_unzFile);
    fileCountInEpub = (size_t)fileCount;
    if (fileCount >= 1) {
        
        // go to first file in zip
        int rtnVal = gotoFirstFile(_unzFile);
        if (rtnVal < 0)
            return filesAndCount;
        
        listArray = (char**)malloc(sizeof(char*)*fileCount);
        
        // loop over the files in zip and add to array
        for (int i= 0; i < fileCount; i++) {
            
            ////struct FilesInZipContainerInfo infoOnFile = getCurrentFileInZipInfo(_unzFile);
            char* infoOnFile = getCurrentFileInZipInfoStr(_unzFile);
            if (infoOnFile == NULL)
            {
                fprintf(stdout, "Error: file is null. Continue to process"); // needs better error checking
                continue;
            }
            
            listArray[i] = (char*)malloc(strlen(infoOnFile)+1);
            
            if (listArray[i] != NULL) {
                ////listArray[i] = strdup(infoOnFile);
                strcpy(listArray[i],infoOnFile);
                free(infoOnFile);
            }
            int err = gotoNextFileInZip(_unzFile);
            if (err != UNZ_OK)
                break;
        }
    }
    
    filesAndCount.arrOfFiles = listArray;
    filesAndCount.fileCount = fileCountInEpub;
    return filesAndCount;
}

uint validateOCF3(const char* epubFilePath, const char* fileSystemRootDir, int writeToDisk)
{    
    /// PASS #1 : check EPUB for files
    char **listArr = NULL;
    
    struct FilesInZipCount filesAndCount;
    filesAndCount.arrOfFiles = listArr;
    
    filesAndCount = listFilesInEpub(epubFilePath);
    
    if (filesAndCount.arrOfFiles != NULL)
    {
        fprintf(stdout, "EPUB file count: %ld\n", (long)filesAndCount.fileCount);
        
        if (filesAndCount.fileCount == 0)
        {
            fprintf(stdout, "EPUB file count is empty. Invalid EPUB.\n");
            freeFilesAndCounts(&filesAndCount);
            return NO_FILES_IN_EPUB;
        }
        
        for(int i=0; i < filesAndCount.fileCount; i++)
        {
            fprintf(stdout, "EPUB container file: %s\n", filesAndCount.arrOfFiles[i]);
        }
    }
    else
    {
        fprintf(stdout, "Error: EPUB file list is NULL.\n");
        freeFilesAndCounts(&filesAndCount);
        return UNKNOWN_ZIP_ERR;
    }
    
    /// PASS #2 : check for mimetype
    // get the unzipped file
    unzFile _unzFile = unzipFile(epubFilePath);
    if (_unzFile == NULL) // punt
    {
        freeFilesAndCounts(&filesAndCount);
        return ZIP_ERR;
    }
    
    uint rtnVal = checkMimetype(&filesAndCount, _unzFile);
    if (rtnVal != NO_ERRORS)
    {
        fprintf(stdout, "Error: EPUB parsing mimetype.\n");
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;
    }
    
    /// PASS #3 : check for required OPF file
    // locate the container.xml file that references OPF files
    rtnVal = locateFileInEpub(_unzFile, &filesAndCount, "META-INF/container.xml");
    if (rtnVal != NO_ERRORS)
    {
        fprintf(stdout, "Error: EPUB No required META-INF/container.xml.\n");
        rtnVal = NO_CONTAINER_XML;
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;
    }
    // read container.xml into mem buffer
    uint bufferSize = 0x800;
    void* byteBuf = (void*)malloc(sizeof(char)*bufferSize);
    rtnVal = readFileFromEpub(_unzFile, byteBuf, bufferSize);
    if (rtnVal < 0)
    {
        fprintf(stdout, "Error: EPUB Unable to read container.xml file.\n");
        freeFilesAndCounts(&filesAndCount);
        free(byteBuf);
        return rtnVal;
    }
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx; 
    // Load XML document
    xmlInitParser();
    doc = xmlReadMemory(byteBuf, rtnVal, "", NULL, XML_PARSE_RECOVER);
    free(byteBuf);
    if (doc == NULL)
    {
        xmlFreeDoc(doc);
        cleanUp();
        fprintf(stdout, "Error: Unable to create xmlDocPtr from META-INF/container.xml.\n");
        rtnVal = XML_READ_MEM_ERR;
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;        
    }
    // Create xpath evaluation context
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        xmlFreeDoc(doc);
        cleanUp();
        fprintf(stdout, "Error: XML unable to create xpath context from xmlDocPtr.\n");
        rtnVal = XPATH_CONTEXT_ERR;
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;      
    }
    
    char *containerNSPrefix = "container";
    char *containerNSHref   = "urn:oasis:names:tc:opendocument:xmlns:container";
    
    if(xmlXPathRegisterNs(xpathCtx, (xmlChar*)containerNSPrefix,
                          (xmlChar*)containerNSHref) != 0) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        cleanUp();
        fprintf(stdout, "Error: XML registering container.xml Namespace with parser.\n");
        rtnVal = XPATH_CONTEXT_NS_ERR;
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;
    }
    
    xmlChar *rootFileFullPathXPathExpr = (xmlChar*)"//container:rootfile";
    
    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEvalExpression(rootFileFullPathXPathExpr,
                                      xpathCtx);
    if(xpathObj == NULL) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        cleanUp();
        fprintf(stdout, "Error: XML unable to locate rootFile in META-INF/container.xml.\n");
        rtnVal = NULL_XPATH_PTR;
        freeFilesAndCounts(&filesAndCount);
        return rtnVal;
    }
    
#ifdef DEBUG
    print_xpath_nodes(xpathObj->nodesetval, stdout);
#endif
    
    // Get the OPF file
    //ex: urn:oasis:names:tc:opendocument:xmlns:container:OPS/package.opf
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    char *OPF_root_dir = NULL;
        
    for(int i = 0; i < size; ++i)
    {
        if(xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
            xmlChar* attrVal = xmlGetProp(xpathObj->nodesetval->nodeTab[i],
                                          (xmlChar*)"full-path");
            if (attrVal != NULL)
            {
                // step 1: get the content documents root
                // hack
                if (strcmp((const char*)attrVal, "content.opf") != 0)
                {
                    char *token_OPF_root_dir = (char*)malloc( strlen((char*)attrVal) + 1 );
                    strcpy(token_OPF_root_dir, (char*)attrVal);                
                    OPF_root_dir = strtok(token_OPF_root_dir, "/");
                }
                
                // step 2: loop over filesAndCount for sanity checking
                // opf file is in container as referenced by META-INF/container.xml
                
                if (locateFileInEpub(_unzFile, &filesAndCount, (char*)attrVal) == UNZ_OK)
                {
                    uint bufferSize = 0xffff;
                    void* opfByteBuf = (void*)malloc(sizeof(char)*bufferSize);
                    rtnVal = readFileFromEpub(_unzFile, opfByteBuf, bufferSize);
                    if (rtnVal < 0)
                    {
                        xmlFree(attrVal);
                        cleanUp();
                        fprintf(stdout, "Error: Unable read into memory OPF file.\n");
                        rtnVal = READ_BYTES_ERR;
                        freeFilesAndCounts(&filesAndCount);
                        free(opfByteBuf);
                        return rtnVal;
                    }
                 
                    rtnVal = parse_opf_file(_unzFile, &opfByteBuf, rtnVal);
                    free(opfByteBuf);
                }
                else
                {
                    xmlFree(attrVal);
                    cleanUp();
                    fprintf(stdout, "Error: Unable to locate OPF file.\n");
                    rtnVal = NO_FILE_BY_NAME;
                    freeFilesAndCounts(&filesAndCount);
                    return rtnVal;
                }
            }
            else
            {
                xmlFree(attrVal);
                cleanUp();
                fprintf(stdout, "Error: XML rootFile node has no full-path attr.\n");
                rtnVal = NO_FULLPATH_ATTR_ROOTFILE;
                freeFilesAndCounts(&filesAndCount);
                return rtnVal;
            }
            xmlFree(attrVal);
            
            // at this point the main_json_object JSON data structure is built. Check writeToDisk for writing manifest
            // content documents to disk
            int manifest_array_length = json_object_array_length(manifest_json_object);
            if (writeToDisk && manifest_array_length > 0)
            {
                rtnVal = writeContentDocumentsToDisk(_unzFile, fileSystemRootDir, (const char*)OPF_root_dir,
                                                     manifest_array_length, &filesAndCount);
                
            } // closing writeToDisk
        }
    } // close loop on OPF

    /* clean up memory */
    freeFilesAndCounts(&filesAndCount);
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    
    return rtnVal;
}

void cleanUpMemory()
{
    // decrement reference count of top level json object
    if (main_json_object != NULL)
        json_object_put(main_json_object);
}

void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output) {
    xmlNodePtr cur;
    int size;
    int i;
    
    assert(output);
    size = (nodes) ? nodes->nodeNr : 0;
    
    fprintf(output, "Result (%d nodes):\n", size);
    for(i = 0; i < size; ++i) {
        assert(nodes->nodeTab[i]);
        
        if(nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            xmlNsPtr ns;
            
            ns = (xmlNsPtr)nodes->nodeTab[i];
            cur = (xmlNodePtr)ns->next;
            if(cur->ns) { 
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s:%s\n", 
                        ns->prefix, ns->href, cur->ns->href, cur->name);
            } else {
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s\n", 
                        ns->prefix, ns->href, cur->name);
            }
        } else if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
            cur = nodes->nodeTab[i];   	    
            if(cur->ns) { 
    	        fprintf(output, "= element node \"%s:%s\"\n", 
                        cur->ns->href, cur->name);
                
                xmlChar* attrVal1 = xmlGetProp(cur, (xmlChar*)"full-path");
                if (attrVal1 != NULL)
                    fprintf(stdout, "attr val: %s\n", (char*)attrVal1);
                xmlFree(attrVal1);
                
                xmlChar* attrVal2 = xmlGetProp(cur, (xmlChar*)"href");
                if (attrVal2 != NULL)
                    fprintf(stdout, "attr val: %s\n", (char*)attrVal2);
                xmlFree(attrVal2);
                
            } else {
    	        fprintf(output, "= element node \"%s\"\n", 
                        cur->name);
            }
        } else {
            cur = nodes->nodeTab[i];
            fprintf(output, "= node \"%s\": type %d\n", cur->name, cur->type);
        }
    }
}