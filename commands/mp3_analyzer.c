#include <stdio.h>
#include "../main.h"
#include <string.h>
#include <portaudio.h>

const int sample_rates[4] = {44100, 48000, 32000, 0};
const int bitrates[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};

float calculate_vbr_bitrate(FILE *fp);
int is_vbr(FILE *fp);
float get_mp3_bitrate(const char *file);
float get_mp3_sample_rate(const char *file);
int mp3_analyzer_check(const char *file);
float get_mp3_sample_rate_from_fp(FILE *fp);

void mp3_analyzer(const char *file)
{
    if (!mp3_analyzer_check(file))
    {
        fprintf(stderr, "Error: file %s not found\n", file);
        return;
    }

    int rate = get_mp3_sample_rate(file);
    float bitrate = get_mp3_bitrate(file);
    if (rate == 0)
    {
        fprintf(stderr, "Error: could not determine sample rate\n");
        return;
    }
    else
    {
        printf("Sample rate: %d Bitrate: %.2f\n", rate, bitrate);
    }
}

int mp3_analyzer_check(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL)
    {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

float get_mp3_sample_rate(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL)
        return 0.0;

    unsigned char header[10];

    if (fread(header, 1, 3, fp) != 3)
    {
        fclose(fp);
        return 0.0;
    }

    if (memcmp(header, "ID3", 3) == 0)
    {
        fseek(fp, 10, SEEK_SET);
        fread(header, 1, 4, fp);
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
        fread(header, 1, 4, fp);
    }

    while (fread(header, 1, 4, fp) == 4)
    {
        if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0)
        {
            int sample_rate_index = (header[2] >> 2) & 0x03;
            fclose(fp);
            return sample_rates[sample_rate_index];
        }
        fseek(fp, -3, SEEK_CUR);
    }

    fclose(fp);
    return 0.0;
}

float get_mp3_sample_rate_from_fp(FILE *fp)
{
    if (!fp)
        return 0.0;

    long current_pos = ftell(fp);
    if (current_pos < 0)
        return 0.0;

    fseek(fp, 0, SEEK_SET);

    unsigned char header[10];
    memset(header, 0, sizeof(header));

    if (fread(header, 1, 3, fp) != 3)
    {
        fseek(fp, current_pos, SEEK_SET);
        return 0.0;
    }

    if (memcmp(header, "ID3", 3) == 0)
    {
        unsigned char size_buffer[4];
        fseek(fp, 6, SEEK_SET);
        if (fread(size_buffer, 1, 4, fp) == 4)
        {
            int tagSize = ((size_buffer[0] & 0x7f) << 21) |
                          ((size_buffer[1] & 0x7f) << 14) |
                          ((size_buffer[2] & 0x7f) << 7) |
                          (size_buffer[3] & 0x7f);
            fseek(fp, tagSize + 10, SEEK_SET);
        }
        else
        {
            fseek(fp, current_pos, SEEK_SET);
            return 0.0;
        }
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
    }

    while (fread(header, 1, 4, fp) == 4)
    {
        if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0)
        {
            int sample_rate_index = (header[2] >> 2) & 0x03;

            fseek(fp, current_pos, SEEK_SET);
            return sample_rates[sample_rate_index];
        }
        fseek(fp, -3, SEEK_CUR);
    }

    fseek(fp, current_pos, SEEK_SET);
    return 0.0;
}

float get_mp3_bitrate(const char *file)
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

    if (is_vbr(fp))
    {
        float bitrate = calculate_vbr_bitrate(fp);
        fclose(fp);
        return bitrate;
    }
    else
    {
        unsigned char frame_header[4];
        while (fread(frame_header, 1, 4, fp) == 4)
        {
            if (frame_header[0] == 0xFF && frame_header[1] == 0xFB)
            {
                int bitrate_index = (frame_header[2] >> 4) & 0x0F;
                float actual_bitrate = bitrates[bitrate_index];

                fclose(fp);
                return actual_bitrate;
            }
            fseek(fp, -3, SEEK_CUR);
        }
    }

    fclose(fp);
    return 0;
}

float calculate_vbr_bitrate(FILE *fp)
{
    unsigned char buffer[8];
    unsigned long frames = 0;
    unsigned long bytes = 0;
    double duration = 0.0;

    fread(buffer, 1, 4, fp);
    unsigned int flag = (buffer[0] << 24) | (buffer[1] << 16) |
                        (buffer[2] << 8) | buffer[3];

    printf("VBR flag: 0x%x\n", flag);

    if (flag & 0x0001)
    {
        fread(buffer, 1, 4, fp);
        frames = ((unsigned long)buffer[0] << 24) |
                 ((unsigned long)buffer[1] << 16) |
                 ((unsigned long)buffer[2] << 8) |
                 buffer[3];
        printf("Frames: %lu\n", frames);
    }

    if (flag & 0x0002)
    {
        fread(buffer, 1, 4, fp);
        bytes = ((unsigned long)buffer[0] << 24) |
                ((unsigned long)buffer[1] << 16) |
                ((unsigned long)buffer[2] << 8) |
                buffer[3];
        printf("Bytes: %lu\n", bytes);
    }

    fseek(fp, 0, SEEK_SET);
    int sample_rate = get_mp3_sample_rate_from_fp(fp);

    duration = (double)frames * 1152.0 / (double)sample_rate;
    if (duration > 0.0)
    {
        double kbps = ((double)bytes * 8.0 / duration) / 1000.0;
        return (float)kbps;
    }

    return 0.0;
}

int is_vbr(FILE *fp)
{
    unsigned char header[4];

    char xing[4];

    fseek(fp, 0, SEEK_SET);

    if (fread(header, 1, 3, fp) == 3 && memcmp(header, "ID3", 3) == 0)
    {
        unsigned char size[4];
        fseek(fp, 6, SEEK_SET);
        fread(size, 1, 4, fp);
        int tagSize = ((size[0] & 0x7f) << 21) |
                      ((size[1] & 0x7f) << 14) |
                      ((size[2] & 0x7f) << 7) |
                      (size[3] & 0x7f);
        fseek(fp, tagSize + 10, SEEK_SET);
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
    }

    while (fread(header, 1, 4, fp) == 4)
    {
        if (header[0] == 0xFF && header[1] == 0xFB)
        {
            long pos = ftell(fp) - 4;
            fseek(fp, pos + 36, SEEK_SET);
            if (fread(xing, 1, 4, fp) == 4)
            {
                if (memcmp(xing, "Xing", 4) == 0 || memcmp(xing, "Info", 4) == 0)
                {
                    return 1;
                }
            }
            break;
        }
        fseek(fp, -3, SEEK_CUR);
    }
    return 0;
}