#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "board_config.h"

/* 固件映像: 8KB flash + 128B EEPROM 整个放 RAM (RP2040 有 264KB) */
typedef struct {
    uint8_t  flash[STM8_FLASH_SIZE];
    uint32_t flash_lo, flash_hi;   /* 使用范围 [lo, hi) */
    uint8_t  eep[STM8_EEP_SIZE];
    bool     eep_used;
} image_t;

void image_init(image_t *img);
bool image_add(image_t *img, uint32_t addr, const uint8_t *data, uint32_t len);
bool hex_parse_mem(image_t *img, const char *buf, uint32_t len, char *err, int errlen);