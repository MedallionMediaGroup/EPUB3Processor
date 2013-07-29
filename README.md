##README - EPUB3Processor

###UPDATE 6/29/13
This is a complete rewrite now merged on master branch! The rewrite provides a cleaner interface with a reference counting memory management model. The new code base works much better on ARM based processors maintaining light memory footprint and improved data structures for data encapsulation.

Read below in Usage and Testing and in API sections for more information and how to integrate into your projects.

###What is EPUB3Processor?
EPUB3Processor is a light-footprint, fast, portable EPUB validating parser compatible with EPUB 2.x and the emerging 3 standard. It provides functions for most tasks in the processing and validation of EPUB while sharing common interface between calls. See EPUB3Ref and EPUB3Error in API section below.

EPUB3Processor is dependent on third party, open source, code listed below. Careful consideration in selecting third party software was important in maintaining performance and portability. See License section for details on distribution.

1. [zlib](http://www.zlib.net/). Widely popular gz (de)compression library.
2. [Minizip](http://www.winimage.com/zLibDll/minizip.html). Widely popular extension to zlib for handling PKZip standard zip compression/deflate. EPUB container standardized on PKZip standard format.
3. libxml2 dynamic library - available on most modern operating systems. A C based low level XML parser.

###What platforms have been tested?
EPUB3Processor is written in pure C language conforming to ANSI 99 standard. It has been tested on Apple Mac OS X (Snow Leopard and Lion), 32 bit mode, operating system with Intel compatible processors. Additionally, testing has been performed on Apple iOS 5.x, 6.x, and  minimally on 7 operating systems. Xcode 4 project files are available in this distribution. EPUB3Processor is light weight and fast and should compile and perform well on other platform such as Linux and Windows operating systems but help is needed to setup projects for these platforms. See documents in distribution under license also under support_libs for the individual support code.

###Usage and Testing

Current Xcode project for EPUB3Processor generates a static library named libEPUB3Processor.a. Link to this library in your client code or simply include and compile the code in your project.

**API - see EPUB3.h**

	/* Creates and returns reference to an EPUB stored at path */
	EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path, EPUB3Error *error);

	/* Memory management */
	void EPUB3Retain(EPUB3Ref epub);
	void EPUB3Release(EPUB3Ref epub);

	/* metadata copy and return functions */
	char * EPUB3CopyTitle(EPUB3Ref epub);
	char * EPUB3CopyIdentifier(EPUB3Ref epub);
	char * EPUB3CopyLanguage(EPUB3Ref epub);
	char * EPUB3CopyCoverImagePath(EPUB3Ref epub);

	/* locates cover image in epub and copies to bytes */
	EPUB3Error EPUB3CopyCoverImage(EPUB3Ref epub, void ** bytes, uint32_t * byteCount);
	
	/* Returns count of linear items in OPF spine */
	int32_t EPUB3CountOfSequentialResources(EPUB3Ref epub);
	/* Adds the href attribute of linear spine resource to array of resources */
	EPUB3Error EPUB3GetPathsOfSequentialResources(EPUB3Ref epub, const char ** resources);
	/* Extracts epub archive to path  */
	EPUB3Error EPUB3ExtractArchiveToPath(EPUB3Ref epub, const char * path);
	/* in container.xml copied rootfile element full-path attribute into rootPath */
	EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath);

	/* TOC functions */
	int32_t EPUB3CountOfTocRootItems(EPUB3Ref epub);
	EPUB3Error EPUB3GetTocRootItems(EPUB3Ref epub, EPUB3TocItemRef *tocItems);
	EPUB3Bool EPUB3TocItemHasParent(EPUB3TocItemRef tocItem);
	EPUB3TocItemRef EPUB3TocItemGetParent(EPUB3TocItemRef tocItem);
	int32_t EPUB3TocItemCountOfChildren(EPUB3TocItemRef tocItem);
	EPUB3Error EPUB3TocItemGetChildren(EPUB3TocItemRef parent, EPUB3TocItemRef *children);
	char * EPUB3TocItemCopyTitle(EPUB3TocItemRef tocItem);
	char * EPUB3TocItemCopyPath(EPUB3TocItemRef tocItem);

Several unit tests are available in TestEPUB3Processor directory. Please reference while developing your own application. Note the previous version is now deprecated but made available under deprecated directory.

###Open Source Contribution
EPUB3Processor was written by [Medallion Media Group](http://www.medallionmediagroup.com), for the purpose of supporting EPUB in their [TREEbook](http://www.thetreebook.com) product. To show support for the open source community and IDPF support effort on the [Readium](http://www.readium.org) project, Medallion is contributing the processor so others can benefit from its usage. EPUB3Processor is free to use and distribute both for personal, academic, and commercial use. It may be modified and changed at will so long as the original license is intact. See License and Distribution below.

###Known Issues
EPUB3Processor has been tested using several non-DRM EPUBs available on [Project Gutenberg](http://www.gutenberg.org/), [epubBooks](http://www.epubbooks.com/), [Medallion Press](http://www.medallionmediagroup.com), and the supplied Moby Dick EPUB 3 example on IDPF.

Issues and bugs will now be tracked in GitHub.

###Licensing and Distribution
EPUB3Processor is distributed under the MIT license. See LICENSE-MIT in distribution for details. Essentially, you are free to use, modify and redistribute this code for personal or commercial use.

###Roadmap
See the projects Wiki on GitHub for future direction of EPUB3Processor.

