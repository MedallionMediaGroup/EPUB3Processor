//
//  main.m
//  CmdLineOCFTester
//
//  Created by Brian Buck on 6/25/12.
//  Copyright (c) 2012 Medallion Press, Inc. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify
//  it under the terms of the MIT license. See LICENSE-MIT for details.
//

#import <Foundation/Foundation.h>

#import <time.h>

#import <libxml/tree.h>
#import <libxml/xmlreader.h>
#import <libxml/parser.h>
#import <libxml/HTMLparser.h>
#import <libxml/xpath.h>
#import <libxml/xpathInternals.h>

#import "EPUB3Processor.h"

// function prototypes
int execTime(const char* filePath, const char* fileSystemRootDir, int (*func)(const void*, const void*));
int listEpubContents(const char* filePath);
int validateEPUB(const char* filePath, const char* fileSystemRootDir);

int main(int argc, const char * argv[])
{
    int rtnVal = 0;
    
    if (argv[1] == NULL || argv[2] == NULL)
        return 1;
    
    /// TEST 1: simple list of files contained in EPUB (zip) ///
    ////rtnVal = listEpubContents(argv[1]);
    
    /// TEST 2: validate entire EPUB ///
    
    rtnVal = execTime(argv[1], argv[2], (int (*)(const void*, const void*))(validateEPUB));
    
    if (rtnVal == NO_ERRORS) {
        // print the json
        fprintf(stdout, "main_json_object = %s\n", getMainJsonObjectAsString());
        
        // CLEAN UP MEMORY FOR JSON OBJECTS AND ARRAY LISTS
        cleanUpMemory();
        
        fprintf(stdout, "TEST 2 PASSES!\n");
    }
    else {
        fprintf(stdout, "TEST 2 FAILED. CODE: %d\n", rtnVal);
    }
         
    return rtnVal;
}

int execTime(const char* filePath, const char* fileSystemRootDir, int (*func)(const void*, const void*))
{
    int rtnVal;
    // print execution elapsed time
    clock_t starttime, endtime;
    
    starttime = clock();
    // CALL TO TEST FUNCTION
    rtnVal = (*func)(filePath, fileSystemRootDir);
    
    endtime = clock();
    double elapsedtimeofexecution = ((double) (endtime - starttime)) / CLOCKS_PER_SEC;
    fprintf(stdout, "time of execution: %f sec.\n", elapsedtimeofexecution);
    
    return rtnVal;
}

int listEpubContents(const char* filePath)
{
    // EXAMPLE: "/Users/bbuck/Documents/Medallion/Projects/CmdLineOCFTester/CmdLineOCFTester/Resources/pg19033-images.epub"
    
    //struct FilesInZipCount filesAndCount;
    struct FilesInZipCount filesAndCount = listFilesInEpub(filePath);
    
    if (filesAndCount.arrOfFiles != NULL)
    {
    
        fprintf(stdout, "EPUB file count: %ld\n", (long)filesAndCount.fileCount);
                
        for(size_t i=0; i < filesAndCount.fileCount; i++)
        {
            fprintf(stdout, "EPUB container file: %s\n", filesAndCount.arrOfFiles[i]);
        }
        
        // free memory
        for(int i=0; i < filesAndCount.fileCount; i++)
        {
            free(filesAndCount.arrOfFiles[i]);
        }
        free(filesAndCount.arrOfFiles);
        
        fprintf(stdout, "TEST 1 PASSES!\n");
    }
    else
        return 1;
    
    //
    return 0;
}

int validateEPUB(const char* filePath, const char* fileSystemRootDir)
{
    return (int)validateOCF3(filePath, fileSystemRootDir, 1); // 1 = writeToDisk
}
