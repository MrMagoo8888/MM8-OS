#include "audio/hda/hda.h"
#include "hal/pci.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "memory.h"
#include "string.h"
#include <arch/i686/paging.h>

#define HDA_GCTL_OFFSET      0x08
#define HDA_CORB_BASE_L      0x40
#define HDA_CORB_BASE_U      0x44
#define HDA_CORB_WRITE_PTR   0x48
#define HDA_CORB_READ_PTR    0x4A
#define HDA_CORB_CTL         0x4C
#define HDA_CORB_SIZE        0x50
#define HDA_RIRB_BASE_L      0x54
#define HDA_RIRB_BASE_U      0x58
#define HDA_RIRB_WRITE_PTR   0x5A
#define HDA_RIRB_READ_PTR    0x5C
#define HDA_RIRB_CTL         0x5E
#define HDA_RIRB_SIZE        0x62
#define HDA_STREAM_BASE      0x80
#define HDA_STREAM_STRIDE    0x20

#define HDA_STREAM_CTL       0x00
#define HDA_STREAM_STS       0x02
#define HDA_STREAM_LPIB      0x04
#define HDA_STREAM_CBL       0x08
#define HDA_STREAM_LVI       0x0C
#define HDA_STREAM_FIFOS     0x10
#define HDA_STREAM_FMT       0x14
#define HDA_STREAM_BDPL      0x18
#define HDA_STREAM_BDPU      0x1C

typedef struct {
    uint32_t addr;
    uint32_t size;
} hda_buffer_desc_t;

static bool g_hda_initialized = false;
static bool g_hda_playing = false;
static volatile uint32_t* g_hda_mmio = 0;
static uint8_t* g_hda_stream_buffer = 0;
static hda_buffer_desc_t* g_hda_bdl = 0;
static uint32_t g_hda_stream_size = 0;

static uint32_t hda_reg_read(uint32_t offset) {
    if (!g_hda_mmio) {
        return 0;
    }
    return g_hda_mmio[offset / 4];
}

static void hda_reg_write(uint32_t offset, uint32_t value) {
    if (!g_hda_mmio) {
        return;
    }
    g_hda_mmio[offset / 4] = value;
}

static bool hda_wait_for_bit(uint32_t offset, uint32_t mask, bool expected, int timeout) {
    while (timeout-- > 0) {
        uint32_t value = hda_reg_read(offset);
        bool matched = (value & mask) != 0;
        if (matched == expected) {
            return true;
        }
    }
    return false;
}

static void hda_wait_for_reset(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if ((hda_reg_read(HDA_GCTL_OFFSET) & 0x01) == 0) {
            return;
        }
    }
}

static void hda_setup_ring_buffers(void) {
    uint8_t* corb = (uint8_t*)malloc(4096);
    uint8_t* rirb = (uint8_t*)malloc(4096);

    if (!corb || !rirb) {
        printf("HDA: failed to allocate CORB/RIRB buffers\n");
        return;
    }

    memset(corb, 0, 4096);
    memset(rirb, 0, 4096);

    hda_reg_write(HDA_CORB_BASE_L, (uint32_t)corb);
    hda_reg_write(HDA_CORB_BASE_U, 0);
    hda_reg_write(HDA_CORB_WRITE_PTR, 0);
    hda_reg_write(HDA_CORB_READ_PTR, 0);
    hda_reg_write(HDA_CORB_CTL, 0x0001);
    hda_reg_write(HDA_CORB_SIZE, 0x0002);

    hda_reg_write(HDA_RIRB_BASE_L, (uint32_t)rirb);
    hda_reg_write(HDA_RIRB_BASE_U, 0);
    hda_reg_write(HDA_RIRB_WRITE_PTR, 0);
    hda_reg_write(HDA_RIRB_READ_PTR, 0);
    hda_reg_write(HDA_RIRB_CTL, 0x0001);
    hda_reg_write(HDA_RIRB_SIZE, 0x0002);

    printf("HDA: ring buffers initialized\n");
}

static void hda_reset_stream(void) {
    uint32_t stream_offset = HDA_STREAM_BASE;
    hda_reg_write(stream_offset + HDA_STREAM_CTL, 0);
    hda_reg_write(stream_offset + HDA_STREAM_CBL, 0);
    hda_reg_write(stream_offset + HDA_STREAM_LVI, 0);
    hda_reg_write(stream_offset + HDA_STREAM_FIFOS, 0);
    hda_reg_write(stream_offset + HDA_STREAM_FMT, 0);
    hda_reg_write(stream_offset + HDA_STREAM_BDPL, 0);
    hda_reg_write(stream_offset + HDA_STREAM_BDPU, 0);
}

void hda_init(pci_device_t* dev) {
    if (g_hda_initialized) {
        return;
    }

    uint32_t bar0 = pci_read_config(dev->bus, dev->device, dev->function, 0x10);
    if (bar0 & 0x1) {
        printf("HDA: BAR0 is an I/O BAR, skipping MMIO setup.\n");
        return;
    }

    uint32_t phys_base = bar0 & 0xFFFFFFF0;
    if (phys_base == 0) {
        printf("HDA: BAR0 did not report a valid base address.\n");
        return;
    }

    uint32_t mmio_page = phys_base & 0xFFFFF000;
    i686_Paging_Map_Range(mmio_page, mmio_page, 0x1000);
    g_hda_mmio = (volatile uint32_t*)mmio_page;

    uint32_t gcap = hda_reg_read(0x00);
    uint32_t gctl = hda_reg_read(HDA_GCTL_OFFSET);
    uint32_t codec_count = (gcap >> 8) & 0x0F;

    printf(
        "HDA: Found controller at %02x:%02x.%d (GCAP=0x%x, codecs=%u)\n",
        dev->bus,
        dev->device,
        dev->function,
        gcap,
        codec_count
    );

    hda_reg_write(HDA_GCTL_OFFSET, gctl | 0x01);
    hda_wait_for_reset();

    if (hda_reg_read(HDA_GCTL_OFFSET) & 0x01) {
        printf("HDA: controller reset did not complete in time.\n");
    } else {
        printf("HDA: controller reset complete.\n");
    }

    hda_setup_ring_buffers();
    hda_reset_stream();

    g_hda_initialized = true;
}

bool hda_play_pcm(const uint8_t* data,
                 uint32_t size,
                 uint16_t channels,
                 uint32_t sample_rate,
                 uint16_t bits_per_sample) {
    if (!g_hda_initialized || !data || size == 0) {
        return false;
    }

    if (bits_per_sample != 8 && bits_per_sample != 16 && bits_per_sample != 24 && bits_per_sample != 32) {
        printf("HDA: unsupported PCM format %u bits/sample\n", bits_per_sample);
        return false;
    }

    if (g_hda_stream_buffer) {
        free(g_hda_stream_buffer);
        g_hda_stream_buffer = 0;
    }

    g_hda_stream_buffer = (uint8_t*)malloc(size);
    if (!g_hda_stream_buffer) {
        printf("HDA: failed to allocate %u bytes for playback buffer\n", size);
        return false;
    }

    memcpy(g_hda_stream_buffer, data, size);
    g_hda_stream_size = size;

    if (!g_hda_bdl) {
        g_hda_bdl = (hda_buffer_desc_t*)malloc(sizeof(hda_buffer_desc_t));
        if (!g_hda_bdl) {
            printf("HDA: failed to allocate buffer descriptor\n");
            return false;
        }
    }

    g_hda_bdl[0].addr = (uint32_t)g_hda_stream_buffer;
    g_hda_bdl[0].size = size;

    uint32_t stream_offset = HDA_STREAM_BASE;
    uint32_t base_format = 0;
    if (bits_per_sample == 8) {
        base_format = 0;
    } else if (bits_per_sample == 16) {
        base_format = 1;
    } else if (bits_per_sample == 24) {
        base_format = 2;
    } else if (bits_per_sample == 32) {
        base_format = 3;
    }

    uint32_t rate_code = 0;
    if (sample_rate >= 48000) {
        rate_code = 6;
    } else if (sample_rate >= 44100) {
        rate_code = 5;
    } else if (sample_rate >= 32000) {
        rate_code = 4;
    } else if (sample_rate >= 22050) {
        rate_code = 3;
    } else if (sample_rate >= 16000) {
        rate_code = 2;
    } else if (sample_rate >= 11025) {
        rate_code = 1;
    }

    uint32_t format = 0;
    format |= (base_format & 0x0F);
    format |= ((channels - 1) & 0x0F) << 4;
    format |= (rate_code & 0x0F) << 20;

    hda_reg_write(stream_offset + HDA_STREAM_CTL, 0);
    hda_reg_write(stream_offset + HDA_STREAM_CBL, size);
    hda_reg_write(stream_offset + HDA_STREAM_LVI, 0);
    hda_reg_write(stream_offset + HDA_STREAM_FMT, format);
    hda_reg_write(stream_offset + HDA_STREAM_BDPL, (uint32_t)g_hda_bdl);
    hda_reg_write(stream_offset + HDA_STREAM_BDPU, 0);
    hda_reg_write(stream_offset + HDA_STREAM_CTL, 0x00000001);

    g_hda_playing = true;
    printf(
        "HDA: playback request accepted (%u Hz, %u-bit, %u channel(s), %u bytes)\n",
        sample_rate,
        bits_per_sample,
        channels,
        size
    );
    printf("HDA: stream registers configured for playback\n");

    return true;
}

void hda_stop_playback(void) {
    g_hda_playing = false;
}

bool hda_is_initialized(void) {
    return g_hda_initialized;
}
