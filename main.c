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

void command_parser(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "h3:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h\t\tShow this help message\n");
            break;
        case '3':
            if (optarg == NULL)
            {
                fprintf(stderr, "Error: -3 option requires a file argument\n");
                break;
            }
            mp3_analyzer(optarg);

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
