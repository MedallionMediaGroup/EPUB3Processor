#include <stdio.h>
#include "EPUB3.h"

#define EPUB3ErrorNone 0



int main(void) {
    EPUB3Error result;
    EPUB3Ref epub = EPUB3CreateWithArchiveAtPath("example.epub", &result);
    if (epub == NULL) {
        fprintf(stderr, "Failed to open EPUB file\n");
        return 1;
    }

    EPUB3Error extractResult = EPUB3ExtractArchiveToPath(epub, "output_directory");
    if (extractResult != EPUB3ErrorNone) {
        fprintf(stderr, "Failed to extract EPUB file\n");
        EPUB3Release(epub);
        return 1;
    }

    char *title = EPUB3CopyTitle(epub);
    char *identifier = EPUB3CopyIdentifier(epub);

    printf("Title: %s\n", title);
    printf("Identifier: %s\n", identifier);

    // Free or release resources as needed
    EPUB3Release(epub);
    return 0;
}
