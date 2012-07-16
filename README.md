##README - EPUB3Processor

###What is EPUB3Processor?
EPUB3Processor is a light-footprint, fast, portable EPUB validating parser compatible with EPUB 2.x and the emerging 3 standard. It provides functions for listing the contents of the EPUB and a single validation entry point api call for validating the structure and syntax of the .epub file. It reports back on validation errors to the caller and optionally writes the manifest content documents to the underlying file system at a location specified by caller. The processor, upon successful validation, provides a function for returning a detailed JSON structure that contains information about the EPUB including its metadata, manifest, and spine reference. The caller can use the JSON structure to provide EPUB compatible reading software details needed to render the publication.

EPUB3Processor is dependent on third party, open source, code listed below. Careful consideration in selecting third party software was important in maintaining performance and portability. See License section for details on distribution.

1. [zlib](http://www.zlib.net/). Widely popular gz (de)compression library.
2. [Minizip](http://www.winimage.com/zLibDll/minizip.html). Widely popular extension to zlib for handling PKZip standard zip compression/deflate. EPUB container standardized on PKZip standard format.
3. [json-c](https://github.com/json-c/json-c). A nice, easy to use, portable JSON library written in C.
4. libxml2 dynamic library - available on most modern operating systems. A C based low level XML parser.

###What platforms have been tested?
EPUB3Processor is written in pure C language conforming to ANSI 99 standard. It has been tested on Apple Mac OS X (Snow Leopard and Lion), 32 bit mode, operating system with Intel compatible processors. Additionally, testing has been performed on Apple iOS 5.x and (redacted) operating systems. As you can tell, Xcode 4 project files are available in this distribution. EPUB3Processor is light weight and fast and should compile and perform well on other platform such as Linux and Windows operating systems but help is needed to setup projects for these platforms. See documents in distribution under license also under support_libs for the individual support code.

###Usage and Testing

Current Xcode project for EPUB3Processor generates a static library named libEPUB3Processor.a. Link to this library in your client code or simply include and compile the code in your project.

**API**

	// returns the parsers internal, top-level json object containing epub metadata,
	// manifest, and spine information.
	const char* getMainJsonObjectAsString();
	
	// returns a structure containing the file count and array of files
	struct FilesInZipCount
	listFilesInEpub(const char* epubFilePath);

	// completely validates the semantical structure of an EPUB, creates an
	// internal json object representing the epub metadata, manifest, and
	// spine information. Optionally writes content documents to fileSystemRootDir
	// if writeToDisk == 1
	uint validateOCF3(const char* epubFilePath,
                  	   const char* fileSystemRootDir,
                  	   int writeToDisk);
                  	   
	// cleans up internal data structure memory used during the listing and validation
	// phases of processing an EPUB
	void cleanUpMemory();

	// debug method for recursively printing XML nodes to output.
	void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output);

Look at main.c in CmdLineOCFTester for usage example. The first argument is the full path to the .epub file to process. The second argument is full path to a read-write accessible file path where the EPUB content documents are written.

EXAMPLE CMD LINE CALL:  
$ ./CmdLineOCFTester "/Users/bbuck/Documents/Medallion/Projects/CmdLineOCFTester/CmdLineOCFTester/Resources/9780316000000\_MobyDick\_r5.epub" /Users/bbuck/EPUB_STAGE/

**note** trailing slash "/" on second command line argument "/Users/bbuck/EPUB_STAGE/"
**Bug if not present.**

Client application is responsible for deleting contents of directory specified in fileSystemRootDir argument to validateOCF3 (if writeToDisk == 1) once caller is done with publication content documents. If not delete contents, subsequent calls with different EPUB (as specified in epubFilePath) will not write files of same name. Recommendation is to delete contents prior to processing another EPUB. Also note value supplied to fileSystemRootDir must be a directory read-writeable (umask 0755) by user of process running calling code.

###Open Source Contribution
EPUB3Processor was written by [Medallion Media Group](http://www.medallionmediagroup.com), for the purpose of supporting EPUB in their [TREEbook](http://www.thetreebook.com) product. To show support for the open source community and IDPF support effort on the [Readium](http://www.readium.org) project, Medallion is contributing the processor so others can benefit from its usage. EPUB3Processor is free to use and distribute both for personal, academic, and commercial use. It may be modified and changed at will so long as the original license is intact. See License and Distribution below.

###Known Issues
EPUB3Processor has been tested using several non-DRM EPUBs available on [Project Gutenberg](http://www.gutenberg.org/), [epubBooks](http://www.epubbooks.com/), [Medallion Press](http://www.medallionmediagroup.com), and the supplied Moby Dick EPUB 3 example on IDPF.

_Here are some known issues:_

1.	Certain metadata may not be processed correctly or may be skipped from the validation and adding to json object. Checks for both EPUB 2.x and 3 metadata are present but more works needs done for robustness.
2.	A few small memory leaks are currently being fixed.
3.	Does not process an NCX only spine data in the OPF.
4.	Validator will loop on all rootfiles specified in contents but has not been fully tested with EPUBS having multiple renditions.
5.	More I'm sure but we will track on GitHub.

###Licensing and Distribution
EPUB3Processor is distributed under the MIT license. See LICENSE-MIT in distribution for details. Essentially, you are free to use, modify and redistribute this code for personal or commercial use.

###Roadmap
See the projects Wiki on GitHub for future direction of EPUB3Processor.

