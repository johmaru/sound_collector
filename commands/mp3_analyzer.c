#include <stdio.h>
#include "../main.h"
#include <string.h>
#include <portaudio.h>
#include <errno.h>

const int sample_rates[4] = {44100, 48000, 32000, 0};
const int bitrates[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
struct MPEGHeader
{
    int version;
    int layer;
    int sample_rate;
};

float calculate_vbr_bitrate(FILE *fp);
int is_vbr(FILE *fp);
float get_mp3_bitrate(const char *file);
float get_mp3_sample_rate(const char *file);
int mp3_analyzer_check(const char *file);
float get_mp3_sample_rate_from_fp(FILE *fp);
int get_header_info(FILE *fp, struct MPEGHeader *header);
int has_id3vv2_tag(FILE *fp);
int get_id3v2_tag_size(FILE *fp);
int get_samples_per_frame(int version, int layer);
long get_audio_position(FILE *fp);
void skip_headers(FILE *fp, long *audio_start_pos);
int has_id3v1_tag(FILE *fp);

void mp3_analyzer(const char *file)
{
    if (!mp3_analyzer_check(file))
    {
        fprintf(stderr, "Error: file %s not found\n", file);
        return;
    }

    FILE *fp = fopen(file, "rb");

    int rate = get_mp3_sample_rate_from_fp(fp);
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

    struct MPEGHeader header;
    double kbps = 0.0;

    long total_bytes = bytes;
    long pos = get_audio_position(fp);
    long audio_data_size = total_bytes - pos;
    printf("Debug - Total bytes: %ld\n", total_bytes);
    printf("Debug - Header position: %ld\n", pos);
    printf("Debug - Audio data size: %ld\n", audio_data_size);

    if (has_id3v1_tag(fp))
    {
        audio_data_size -= 128;
        printf("Debug - Removed ID3v1 tag (128 bytes)\n");
    }

    if (get_header_info(fp, &header))
    {
        int samples_per_frame = get_samples_per_frame(header.version, header.layer);
        printf("Debug - Samples per frame: %d\n", samples_per_frame);

        duration = (double)frames * samples_per_frame / sample_rate;
        printf("Debug - Duration calculation:\n");
        printf("  Frames: %lu\n", frames);
        printf("  Sample rate: %d\n", sample_rate);
        printf("  Duration: %.2f seconds\n", duration);

        double encoded_kbps = ((double)audio_data_size * 8.0 / duration) / 1000.0;
        printf("Debug - Audio data size: %ld bytes\n", audio_data_size);
        printf("Debug - Encoded bitrate: %.2f kbps\n", encoded_kbps);

        return (float)encoded_kbps;
    }

    return kbps;
}

long get_audio_position(FILE *fp)
{
    long audio_start_pos = 0;
    skip_headers(fp, &audio_start_pos);
    return audio_start_pos;
}

void skip_headers(FILE *fp, long *audio_start_pos)
{
    if (!fp || !audio_start_pos)
        return;

    if (has_id3vv2_tag(fp))
    {
        int tag_size = get_id3v2_tag_size(fp);
        fseek(fp, tag_size, SEEK_SET);
    }

    unsigned char buffer[4];
    while (fread(buffer, 1, 4, fp) == 4)
    {

        if (memcmp(buffer, "Xing", 4) == 0)
        {
            fread(buffer, 1, 4, fp);
            unsigned int flags = (buffer[0] << 24) | (buffer[1] << 16) |
                                 (buffer[2] << 8) | buffer[3];

            long xing_size = 8;

            if (flags & 0x0001)
                xing_size += 4;
            if (flags & 0x0002)
                xing_size += 4;
            if (flags & 0x0004)
                xing_size += 100;
            if (flags & 0x0008)
                xing_size += 4;

            fseek(fp, xing_size, SEEK_SET);
        }

        if (buffer[0] == 0xFF && buffer[1] == 0xFB)
        {
            int bitrate_index = (buffer[2] >> 4) & 0x0F;
            int sample_rate_index = (buffer[2] >> 2) & 0x03;

            int frame_size = 144 * bitrates[bitrate_index] * 1000 / sample_rates[sample_rate_index];
            fseek(fp, frame_size, SEEK_CUR);
            break;
        }

        fseek(fp, -3, SEEK_CUR);
    }
    printf("Debug - Audio start position: %ld\n", ftell(fp));

    *audio_start_pos = ftell(fp);
}

int has_id3v1_tag(FILE *fp)
{
    unsigned char tag[3];
    long current_pos = ftell(fp);

    fseek(fp, -128, SEEK_END);

    if (fread(tag, 1, 3, fp) != 3)
    {
        fseek(fp, current_pos, SEEK_SET);
        return 0;
    }

    fseek(fp, current_pos, SEEK_SET);
    return (memcmp(tag, "TAG", 3) == 0);
}

int has_id3vv2_tag(FILE *fp)
{
    unsigned char id3[3];
    long current_pos = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    if (fread(id3, 1, 3, fp) != 3)
    {
        fseek(fp, current_pos, SEEK_SET);
        return 0;
    }

    fseek(fp, current_pos, SEEK_SET);
    return (memcmp(id3, "ID3", 3) == 0);
}

int get_id3v2_tag_size(FILE *fp)
{
    unsigned char size_buffer[4];
    long current_pos = ftell(fp);

    fseek(fp, 6, SEEK_SET);
    if (fread(size_buffer, 1, 4, fp) != 4)
    {
        fseek(fp, current_pos, SEEK_SET);
        return 0;
    }

    int tagSize = ((size_buffer[0] & 0x7f) << 21) |
                  ((size_buffer[1] & 0x7f) << 14) |
                  ((size_buffer[2] & 0x7f) << 7) |
                  (size_buffer[3] & 0x7f);

    fseek(fp, current_pos, SEEK_SET);
    return tagSize + 10;
}

int get_header_info(FILE *fp, struct MPEGHeader *header)
{
    if (!fp || !header)
        return 0;

    long current_pos = ftell(fp);
    if (current_pos < 0)
        return 0;

    fseek(fp, 0, SEEK_SET);

    unsigned char id3[10];
    if (fread(id3, 1, 3, fp) == 3 && memcmp(id3, "ID3", 3) == 0)
    {
        fread(id3 + 3, 1, 7, fp);
        int tagSize = ((id3[6] & 0x7f) << 21) |
                      ((id3[7] & 0x7f) << 14) |
                      ((id3[8] & 0x7f) << 7) |
                      (id3[9] & 0x7f);
        fseek(fp, tagSize + 10, SEEK_SET);
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
    }

    while (fread(id3, 1, 4, fp) == 4)
    {
        if (id3[0] == 0xFF)
        {
            int top4 = (id3[1] >> 4) & 0x0F;
            printf("Debug - Top 4 bits: 0x%x\n", top4);
            int actual_version = (top4 & 0x03);
            int buttom4 = id3[1] & 0x0F;
            printf("Debug - Buttom 4 bits: 0x%x\n", buttom4);
            int actual_layer = (buttom4 >> 2) & 0x03;

            header->version = actual_version;
            header->layer = actual_layer;
            header->sample_rate = get_mp3_sample_rate_from_fp(fp);
            printf("Debug - Version: %d, Layer: %d, Sample Rate: %d\n",
                   header->version, header->layer, header->sample_rate);
            fseek(fp, current_pos, SEEK_SET);
            return 1;
        }
        fseek(fp, -3, SEEK_CUR);
    }

    fseek(fp, current_pos, SEEK_SET);
    return 0;
}

int get_samples_per_frame(int version, int layer)
{
    // MPEG Version 1
    if (version == 3)
    { // MPEG1
        switch (layer)
        {
        case 1: // Layer III
            return 1152;
        case 2: // Layer II
            return 1152;
        case 3: // Layer I
            return 384;
        }
    }
    // MPEG Version 2 & 2.5
    else
    {
        switch (layer)
        {
        case 3: // Layer I
            return 384;
        case 2: // Layer II
            return 1152;
        case 1: // Layer III
            return 576;
        default:
            return 0;
        }
    }
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