#include <stdio.h>
#include "main.h"
#include "commands/mp3_analyzer.h"
#include <string.h>
#include <unistd.h>
#include <getopt.h>

char os[32] = {0};

int main(int argc, char *argv[])
{
    if (!os_check())
        return 1;

    command_parser(argc, argv);

    return 0;
}

static struct option long_options[] =
    {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {"ext", required_argument, 0, 'e'},
        {0, 0, 0, 0}};

void command_parser(int argc, char *argv[])
{
    int opt;
    const char *ext = NULL;
    while ((opt = getopt(argc, argv, "hf:e:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h\t\tShow this help message\n");
            break;
        case 'f':
            if (optarg == NULL)
            {
                fprintf(stderr, "Error: -f option requires a file argument\n");
                break;
            }
            if (ext)
            {
                if (strcmp(ext, "mp3") == 0)
                {
                    mp3_analyzer(optarg);
                }
                else
                {
                    fprintf(stderr, "Error: unsupported file extension\n");
                }
            }
            mp3_analyzer(optarg);
            break;
        case 'e':
            if (optarg == NULL)
            {
                fprintf(stderr, "Error: -e option requires an extension argument\n");
                break;
            }
            ext = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-h]\n", argv[0]);
            break;
        }
    }
}

int os_check()
{
#if defined(_WIN32) || defined(_WIN64)
    strcpy(os, "Windows");
#elif defined(__APPLE__) || defined(__MACH__)
    strcpy(os, "Mac OS X");
#elif defined(__linux__)
    strcpy(os, "Linux");
#elif defined(__FreeBSD__)
    strcpy(os, "FreeBSD");
#elif defined(__unix) || defined(__unix__)
    strcpy(os, "Unix");
#else
    strcpy(os, "Unknown");
#endif
    return 1;
}

const char *os_name()
{
    return os;
}
