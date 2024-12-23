#include <stdio.h>
#include "../main.h"
#include <string.h>
#include <portaudio.h>

const int sample_rates[4] = {44100, 48000, 32000, 0};

void mp3_analyzer(const char *file)
{
    if (!mp3_analyzer_check(file))
    {
        fprintf(stderr, "Error: file %s not found\n", file);
        return;
    }

    int rate = get_mp3_sample_rate(file);
    if (rate == 0)
    {
        fprintf(stderr, "Error: could not determine sample rate\n");
        return;
    }
    else
    {
        printf("Sample rate: %d\n", rate);
    }
}

int mp3_analyzer_check(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL)
    {
        return 0;
    }
    return 1;
}

int get_mp3_sample_rate(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL)
    {
        return 0;
    }

    unsigned char header[10];

    if (fread(header, 1, 3, fp) != 3)
    {
        fclose(fp);
        return 0;
    }

    if (memcmp(header, "ID3", 3) == 0)
    {
        fseek(fp, 128, SEEK_SET);
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
    }

    unsigned char frame_header[4];
    while (fread(frame_header, 1, 4, fp) == 4)
    {
        if (frame_header[0] == 0xFF && frame_header[1] == 0xFB)
        {
            int sample_rate_index = (frame_header[2] >> 2) & 0x03;
            fclose(fp);
            return sample_rates[sample_rate_index];
        }
        fseek(fp, -3, SEEK_CUR);
    }

    fclose(fp);
    return 0;
}