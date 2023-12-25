#include "9cc.h"

#define MAX_BUFFER_SIZE 4096

char *read_file(char *path)
{
    FILE *fp;
    // printf("path %s\n", path);
    if (strcmp(path, "-") == 0)
    {
        // By convention, read from stdin if a given filename is "-".
        fp = stdin;
    }
    else
    {
        fp = fopen(path, "r");
        if (!fp)
            error("cannot open %s: %s", path, strerror(errno));
    }

    char buffer[MAX_BUFFER_SIZE];
    size_t total_size = 0;
    size_t read_size;

    // Read file content in chunks
    char *content = NULL;
    while ((read_size = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        content = realloc(content, total_size + read_size + 1);
        if (!content)
        {
            error("Memory allocation error");
        }

        memcpy(content + total_size, buffer, read_size);
        total_size += read_size;
    }

    if (ferror(fp))
    {
        error("Error reading from %s: %s", path, strerror(errno));
    }

    // Ensure null termination
    content = realloc(content, total_size + 1);
    if (!content)
    {
        error("Memory allocation error");
    }
    content[total_size] = '\0';

    // Close the file if not stdin
    if (fp != stdin)
    {
        fclose(fp);
    }

    return content;
}