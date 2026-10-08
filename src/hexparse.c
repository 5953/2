/* Intel HEX 解析 (类型 00 数据 / 01 结束 / 02 段地址 / 04 线性地址) */
#include "hexparse.h"
#include <stdio.h>
#include <string.h>

void image_init(image_t *img) {
    memset(img->flash, 0xFF, sizeof img->flash);
    memset(img->eep,   0xFF, sizeof img->eep);
    img->flash_lo = STM8_FLASH_BASE + STM8_FLASH_SIZE;
    img->flash_hi = STM8_FLASH_BASE;
    img->eep_used = false;
}

bool image_add(image_t *img, uint32_t addr, const uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++, addr++) {
        if (addr >= STM8_FLASH_BASE && addr < STM8_FLASH_BASE + STM8_FLASH_SIZE) {
            img->flash[addr - STM8_FLASH_BASE] = data[i];
            if (addr < img->flash_lo)     img->flash_lo = addr;
            if (addr + 1 > img->flash_hi) img->flash_hi = addr + 1;
        } else if (addr >= STM8_EEP_BASE && addr < STM8_EEP_BASE + STM8_EEP_SIZE) {
            img->eep[addr - STM8_EEP_BASE] = data[i];
            img->eep_used = true;
        }
        /* 其它区域(如 option bytes 0x4800)直接忽略 */
    }
    return true;
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static int hex2(const char *p) { return (hexval(p[0]) << 4) | hexval(p[1]); }

bool hex_parse_mem(image_t *img, const char *buf, uint32_t len, char *err, int errlen) {
    uint32_t pos = 0, base = 0;
    int lineno = 0;

    while (pos < len) {
        uint32_t start = pos;
        while (pos < len && buf[pos] != '\n') pos++;
        uint32_t llen = pos - start;
        pos++;
        lineno++;
        const char *l = buf + start;
        while (llen && (l[llen - 1] == '\r' || l[llen - 1] == ' ')) llen--;
        if (!llen) continue;

        if (l[0] != ':' || llen < 11) {
            snprintf(err, errlen, "BAD LINE %d", lineno);
            return false;
        }
        int count = hex2(l + 1);
        int addr  = (hex2(l + 3) << 8) | hex2(l + 5);
        int type  = hex2(l + 7);
        if (llen < 11u + (uint32_t)count * 2u) {
            snprintf(err, errlen, "SHORT LINE %d", lineno);
            return false;
        }
        uint8_t sum = 0;
        for (int i = 0; i < 5 + count; i++) sum += (uint8_t)hex2(l + 1 + i * 2);
        if (sum != 0) {
            snprintf(err, errlen, "CKSUM LINE %d", lineno);
            return false;
        }

        if (type == 0x00) {                      /* 数据记录 */
            uint8_t tmp[256];
            for (int i = 0; i < count; i++) tmp[i] = (uint8_t)hex2(l + 9 + i * 2);
            image_add(img, base + (uint32_t)addr, tmp, (uint32_t)count);
        } else if (type == 0x01) {
            return true;                         /* EOF */
        } else if (type == 0x02) {
            base = ((uint32_t)((hex2(l + 9) << 8) | hex2(l + 11))) << 4;
        } else if (type == 0x04) {
            base = ((uint32_t)((hex2(l + 9) << 8) | hex2(l + 11))) << 16;
        }
        /* 03/05 起始地址等忽略 */
    }
    snprintf(err, errlen, "NO EOF RECORD");
    return false;
}