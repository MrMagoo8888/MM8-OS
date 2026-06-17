#include "randomBits/wav/wav.h"
#include "memory.h"
#include "stdio.h"
#include "fat.h"
#include "globals.h"

static uint16_t read_u16_le(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t* p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint32_t align_to_even(uint32_t value) {
    return value + (value & 1);
}

bool wav_is_valid(const WAVInfo* info) {
    return info != 0 && info->data != 0 && info->data_size > 0;
}

uint32_t wav_get_frame_count(const WAVInfo* info) {
    if (!wav_is_valid(info)) {
        return 0;
    }

    uint32_t bytes_per_frame = (info->bits_per_sample / 8) * info->channels;
    if (bytes_per_frame == 0) {
        return 0;
    }

    return info->data_size / bytes_per_frame;
}

void wav_free(WAVInfo* info) {
    if (!info) {
        return;
    }
    if (info->data) {
        free(info->data);
        info->data = 0;
    }
    info->data_size = 0;
    info->audio_format = 0;
    info->channels = 0;
    info->sample_rate = 0;
    info->bits_per_sample = 0;
}

bool wav_parse_buffer(const uint8_t* buffer, uint32_t size, WAVInfo* out) {
    if (!buffer || !out || size < 44) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WAVE", 4) != 0) {
        return false;
    }

    uint32_t offset = 12;
    bool got_fmt = false;
    bool got_data = false;

    while (offset + 8 <= size) {
        const char* chunk_id = (const char*)(buffer + offset);
        uint32_t chunk_size = read_u32_le(buffer + offset + 4);
        uint32_t chunk_data = offset + 8;
        uint32_t next_offset = chunk_data + chunk_size;

        if (next_offset > size) {
            return false;
        }

        if (memcmp(chunk_id, "fmt ", 4) == 0 && chunk_size >= 16) {
            const uint8_t* fmt = buffer + chunk_data;
            out->audio_format = read_u16_le(fmt + 0);
            out->channels = read_u16_le(fmt + 2);
            out->sample_rate = read_u32_le(fmt + 4);
            out->bits_per_sample = read_u16_le(fmt + 14);
            got_fmt = true;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            out->data = (uint8_t*)(buffer + chunk_data);
            out->data_size = chunk_size;
            got_data = true;
            if (got_fmt) {
                break;
            }
        }

        offset = next_offset + (chunk_size & 1);
    }

    if (!got_fmt || !got_data || out->data_size == 0 || out->audio_format != 1) {
        return false;
    }

    return true;
}

bool wav_parse_file(const char* path, WAVInfo* out) {
    if (!path || !out) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_READ);
    if (!file) {
        printf("WAV: Could not open %s\n", path);
        return false;
    }

    uint32_t size = file->Size;
    if (size < 44) {
        printf("WAV: %s is too small to be a valid WAV file\n", path);
        FAT_Close(&g_Disk, file);
        return false;
    }

    uint8_t* file_buffer = (uint8_t*)malloc(size);
    if (!file_buffer) {
        printf("WAV: Failed to allocate %u bytes\n", size);
        FAT_Close(&g_Disk, file);
        return false;
    }

    if (FAT_Read(&g_Disk, file, size, file_buffer) != size) {
        printf("WAV: Failed to read %s\n", path);
        free(file_buffer);
        FAT_Close(&g_Disk, file);
        return false;
    }

    FAT_Close(&g_Disk, file);

    WAVInfo temp = {0};
    if (!wav_parse_buffer(file_buffer, size, &temp)) {
        printf("WAV: %s is not a supported PCM WAV file\n", path);
        free(file_buffer);
        return false;
    }

    if (temp.data_size > 0 && temp.data != 0) {
        out->data = (uint8_t*)malloc(temp.data_size);
        if (!out->data) {
            printf("WAV: Failed to allocate %u bytes for audio data\n", temp.data_size);
            free(file_buffer);
            return false;
        }
        memcpy(out->data, temp.data, temp.data_size);
        out->data_size = temp.data_size;
    }

    out->audio_format = temp.audio_format;
    out->channels = temp.channels;
    out->sample_rate = temp.sample_rate;
    out->bits_per_sample = temp.bits_per_sample;

    free(file_buffer);
    return true;
}
