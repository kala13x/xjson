/*
 *  xjson/xjson.c
 *
 *  This source is part of "xjson" project
 *  2019-2021  Sun Dro (f4tb0y@protonmail.com)
 * 
 * Parse, lint and minify json using xjson library.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include "lib/xjson.h"

extern char *optarg;

#define XJSON_LINT_VER_MAX  0
#define XJSON_LINT_VER_MIN  1

#define XJSON_CLR_RED       "\x1B[31m"
#define XJSON_CLR_RESET     "\033[0m"

typedef struct xjson_args_ {
    char sFile[PATH_MAX];
    uint8_t nMinify:1;
    size_t nTabSize;
} xjson_args_t;

void display_usage(const char *pName)
{
    printf("================================================\n");
    printf(" Lint and Minify JSON file - v%d.%d (%s)\n",
        XJSON_LINT_VER_MAX, XJSON_LINT_VER_MIN, __DATE__);
    printf("================================================\n");

    printf("Usage: %s [-f <file>] [-l <size>] [-m] [-h]\n\n", pName);
    printf("Options are:\n");
    printf("  -f <file>           # JSON file path (%s*%s)\n", XJSON_CLR_RED, XJSON_CLR_RESET);
    printf("  -l <size>           # Linter tab size\n");
    printf("  -m                  # Minify json file\n");
    printf("  -h                  # Version and usage\n\n");
    printf("Example: %s -f example.json -l 4\n", pName);
}

int parse_args(xjson_args_t *pArgs, int argc, char *argv[])
{
    pArgs->nTabSize = 4;
    pArgs->nMinify = 0;
    pArgs->sFile[0] = 0;

    int nChar = 0;
    while ((nChar = getopt(argc, argv, "f:l:m1:h1")) != -1) 
    {
        switch (nChar)
        {
            case 'f':
                snprintf(pArgs->sFile, sizeof(pArgs->sFile), "%s", optarg);
                break;
            case 'l':
                pArgs->nTabSize = atoi(optarg);
                break;
            case 'm':
                pArgs->nMinify = 1;
                break;
            case 'h':
            default:
                return 0;
        }
    }

    return (pArgs->sFile[0] != 0) ? 1 : 0;
}

uint8_t* load_file(const char *pFile, size_t *pSize)
{
    int nFD = open(pFile, O_RDONLY);
    if (nFD < 0)
    {
        printf("Can't open file: %s (%s)\n", 
            pFile, strerror(errno));
        return NULL;
    }

    size_t nSize = lseek(nFD, 0, SEEK_END);
    lseek(nFD, 0, SEEK_SET);

    uint8_t *pBuffer = (uint8_t*)malloc(nSize + 1);
    if (pBuffer == NULL)
    {
        printf("Can't allocate memory for file: %s (%s)\n", 
            pFile, strerror(errno));

        close(nFD);
        return NULL;
    }

    int nBytes = read(nFD, pBuffer, nSize);
    if (nBytes <= 0)
    {
        free(pBuffer);
        close(nFD);
        return NULL;
    }

    *pSize = (nBytes > 0) ? nBytes : 0;
    pBuffer[*pSize] = '\0';
    close(nFD);

    return pBuffer;
}

int main(int argc, char *argv[])
{
    xjson_args_t args;
    xjson_t json;
    size_t nSize;

    if (!parse_args(&args, argc, argv))
    {
        display_usage(argv[0]);
        return 1;
    }

    char *pBuffer = (char*)load_file(args.sFile,  &nSize);
    if (pBuffer == NULL)
    {
        printf("Can't read file: %s (%s)\n", args.sFile, strerror(errno));
        return 1;
    }

    if (!XJSON_Parse(&json, pBuffer, nSize))
    {
        char sError[256];
        XJSON_GetErrorStr(&json, sError, sizeof(sError));
        printf("Failed to parse JSON: %s\n", sError);

        XJSON_Destroy(&json);
        free(pBuffer);
        return 1;
    }

    if (args.nMinify)
    {
        /* Dump json from xjson_t object */
        if (XJSON_Write(&json, pBuffer, nSize + 1)) printf("%s\n", pBuffer);
        else printf("Can not minify json: errno(%d)\n", errno);
    }
    else
    {
        free(pBuffer);
        pBuffer = NULL;

        xjson_writer_t writer;
        XJSON_InitWriter(&writer, NULL, nSize); // Dynamic allocation
        writer.nTabSize = args.nTabSize; // Enable linter and set tab size

        /* Dump objects directly */
        if (XJSON_WriteObject(json.pRootObj, &writer)) printf("%s\n", writer.pData);
        else printf("Can not lint json: errno(%d): %s\n", errno, writer.pData ? writer.pData : "");

        XJSON_DestroyWriter(&writer);
    }

    XJSON_Destroy(&json);
    free(pBuffer);
    return 0;
}