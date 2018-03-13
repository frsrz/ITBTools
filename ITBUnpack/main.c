#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#if defined (__unix__) || defined (__APPLE__)
    #include <sys/types.h>
    #include <sys/stat.h>
#elif defined(_WIN32)
    #include <direct.h>
    #define mkdir(A, B) _mkdir(A)
#else
    #error "Could not determine platform. Please define __GNUC__ or __MINGW32__"
#endif

static const char * const K_INFO_USAGE = "Usage: ITBUnpack resource.dat\n";
static const char * const K_ERROR_CANNOT_OPEN = "Error: Cannot open specified file\n";
static const char * const K_ERROR_EOF = "Error: EOF was reached while reading\n";
static const char * const K_ERROR_MEMORY = "Error: Failed to allocate memory\n";
static const char * const K_ERROR_FNAME_2LONG = "Error: Filename is too long for the current file system\n";
static const char * const K_ERROR_CANNOT_MKDIR = "Error: Cannot create directory\n";
static const char * const K_ERROR_WRITE = "Error: Cannot write to output file\n;";

int makeDirectoryTree(const char *path, const char *separator);

int main(int argc, char **argv)
{
    FILE *dat;
    uint32_t fileCount;
    uint32_t fileNameLen;
    uint32_t fileSize;
    char *fileName;
    FILE *dst;
    char copyBuffer[1024];
    int exitCode = EXIT_FAILURE;

    if (argc != 2)
    {
        printf(K_INFO_USAGE);
        goto END0;
    }

    if ((dat = fopen(argv[1], "rb")) == NULL)
    {
        printf(K_ERROR_CANNOT_OPEN);
        goto END0;
    }

    if (fread(&fileCount, sizeof(fileCount), 1, dat) != 1)
    {
        printf(K_ERROR_EOF);
        goto END1;
    }

    printf("Unpacking %u files\n", fileCount);

    if (fseek(dat, fileCount * sizeof(uint32_t), SEEK_CUR) != 0)
    {
        printf(K_ERROR_EOF);
        goto END1;
    }

    if ((fileName = (char *) malloc(FILENAME_MAX + 1)) == NULL)
    {
        printf(K_ERROR_MEMORY);
        goto END1;
    }

    for (uint32_t i = 0; i < fileCount; i++)
    {
        if (fread(&fileSize, sizeof(fileSize), 1, dat) != 1)
        {
            printf(K_ERROR_EOF);
            goto END2;
        }

        if (fread(&fileNameLen, sizeof(fileNameLen), 1, dat) != 1)
        {
            printf(K_ERROR_EOF);
            goto END2;
        }

        if (fileNameLen > FILENAME_MAX)
        {
            printf(K_ERROR_FNAME_2LONG);
            goto END2;
        }

        if (fread(fileName, sizeof(char), fileNameLen, dat) != fileNameLen)
        {
            printf(K_ERROR_EOF);
            goto END2;
        }
        fileName[fileNameLen] = '\0';
        printf("%s\n", fileName);

        if (makeDirectoryTree(fileName, "/") != 0)
        {
            printf(K_ERROR_CANNOT_MKDIR);
            goto END2;
        }

        if ((dst = fopen(fileName, "wb")) == NULL)
        {
            printf(K_ERROR_CANNOT_OPEN);
            goto END2;
        }

        uint32_t remaining = fileSize;
        while (remaining > 0)
        {
            uint32_t transfer = remaining >= sizeof(copyBuffer) ? sizeof(copyBuffer) : remaining;
            if (fread(copyBuffer, transfer, 1, dat) != 1)
            {
                printf(K_ERROR_EOF);
                goto END3;
            }
            if (fwrite(copyBuffer, transfer, 1, dst) != 1)
            {
                printf(K_ERROR_WRITE);
                goto END3;
            }
            remaining -= transfer;
        }
        fclose(dst);
    }

    exitCode = EXIT_SUCCESS;
    goto END2;

    END3:
    fclose(dst);
    END2:
    free(fileName);
    END1:
    fclose(dat);
    END0:
    return exitCode;
}

int makeDirectoryTree(const char *path, const char *separator)
{
    char *curpath = (char *) path;
    char *nextsep;
    char *dirname;
    int exitCode = 0;

    if (path == NULL || separator == NULL)
    {
        exitCode = EINVAL;
        goto END0;
    }

    if (strlen(path) > FILENAME_MAX)
    {
        exitCode = ENAMETOOLONG;
        goto END0;
    }

    if ((dirname = (char *) malloc(FILENAME_MAX + 1)) == NULL)
    {
        exitCode = ENOMEM;
        goto END0;
    }

    while (true)
    {
        nextsep = strstr(curpath, separator);
        if (nextsep == NULL)
        {
            exitCode = 0;
            goto END1;
        }

        memcpy(dirname, path, nextsep - path);
        dirname[nextsep - path] = '\0';

        if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && errno != EEXIST)
        {
            exitCode = errno;
            goto END1;
        }

        curpath = nextsep + strlen(separator);
    }

    END1:
    free(dirname);
    END0:
    return exitCode;
}
