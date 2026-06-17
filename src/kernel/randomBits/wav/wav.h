#pragma once

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint32_t data_size;
    uint8_t* data;
} WAVInfo;

bool wav_parse_buffer(const uint8_t* buffer, uint32_t size, WAVInfo* out);
bool wav_parse_file(const char* path, WAVInfo* out);
void wav_free(WAVInfo* info);
bool wav_is_valid(const WAVInfo* info);
uint32_t wav_get_frame_count(const WAVInfo* info);
