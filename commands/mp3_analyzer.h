#ifndef MP3_ANALYZER_H
#define MP3_ANALYZER_H
#include <stdio.h>

void mp3_analyzer(const char *file);
int mp3_analyzer_check(const char *file);
float get_mp3_sample_rate(const char *file);
float get_mp3_bitrate(const char *file);
int is_vbr(FILE *fp);
float calculate_vbr_bitrate(FILE *fp);

#endif