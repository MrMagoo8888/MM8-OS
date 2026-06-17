#pragma once

#include "stdbool.h"
#include "stdint.h"
#include "hal/pci.h"

void hda_init(pci_device_t* dev);
bool hda_is_initialized(void);
bool hda_play_pcm(const uint8_t* data,
                 uint32_t size,
                 uint16_t channels,
                 uint32_t sample_rate,
                 uint16_t bits_per_sample);
void hda_stop_playback(void);
