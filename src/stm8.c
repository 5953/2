/* STM8S003 烧录流程 (flash 寄存器定义见 PM0051) */
#include "stm8.h"
#include "swim.h"
#include <string.h>

#define R_CR2   0x505B
#define R_IAPSR 0x505F
#define R_PUKR  0x5062
#define R_DUKR  0x5063

#define CR2_PRG    0x01    /* 标准块编程 */
#define CR2_ERASE  0x20    /* 块擦除 */
#define CR2_WPRG   0x40    /* 字编程 */

#define IAPSR_PUL  0x02
#define IAPSR_EOP  0x04
#define IAPSR_DUL  0x08

#define FLASH_BLOCKS (STM8_FLASH_SIZE / STM8_FLASH_BLOCK)   /* 64 块 */

bool stm8_connect(void) { return swim_connect(); }

void stm8_disconnect_run(void) {
    swim_srst();
    swim_release_target();
}

static bool clear_eop(void) {                 /* EOP 写 0 清除, 保留 PUL/DUL */
    uint8_t v;
    if (!swim_read8(R_IAPSR, &v)) return false;
    return swim_write8(R_IAPSR, (uint8_t)(v & ~IAPSR_EOP));
}

static bool wait_eop(void) {                  /* 每次轮询本身耗时 ~0.6ms */
    for (int i = 0; i < 400; i++) {
        uint8_t v;
        if (!swim_read8(R_IAPSR, &v)) return false;
        if (v & IAPSR_EOP) return true;
    }
    return false;
}

bool stm8_unlock(void) {
    uint8_t v;
    if (!swim_write8(R_PUKR, 0x56)) return false;
    if (!swim_write8(R_PUKR, 0xAE)) return false;
    if (!swim_read8(R_IAPSR, &v) || !(v & IAPSR_PUL)) return false;  /* 可能 ROP */
    if (!swim_write8(R_DUKR, 0xAE)) return false;   /* 注意: EEPROM 密钥顺序相反 */
    if (!swim_write8(R_DUKR, 0x56)) return false;
    if (!swim_read8(R_IAPSR, &v) || !(v & IAPSR_DUL)) return false;
    return true;
}

void stm8_lock(void) {
    uint8_t v;
    if (swim_read8(R_IAPSR, &v))
        swim_write8(R_IAPSR, (uint8_t)(v & ~(IAPSR_PUL | IAPSR_DUL)));
}

bool stm8_erase_flash(prog_cb_t cb) {
    static const uint8_t zero[4] = {0, 0, 0, 0};
    if (!swim_write8(R_CR2, CR2_ERASE)) return false;
    for (int b = 0; b < FLASH_BLOCKS; b++) {
        if (!clear_eop()) goto fail;
        /* 向块内任意字地址写 4 个 0 即触发该块擦除 */
        if (!swim_wotf(STM8_FLASH_BASE + (uint32_t)b * STM8_FLASH_BLOCK, zero, 4)) goto fail;
        if (!wait_eop()) goto fail;
        if (cb) cb((b + 1) * 100 / FLASH_BLOCKS);
    }
    swim_write8(R_CR2, 0);
    return true;
fail:
    swim_write8(R_CR2, 0);
    return false;
}

bool stm8_program(const image_t *img, prog_cb_t cb) {
    /* --- 程序区: 标准块编程 (128 字节/块) --- */
    if (img->flash_hi > img->flash_lo) {
        int b0 = (int)((img->flash_lo - STM8_FLASH_BASE) / STM8_FLASH_BLOCK);
        int b1 = (int)((img->flash_hi - 1 - STM8_FLASH_BASE) / STM8_FLASH_BLOCK);
        if (!swim_write8(R_CR2, CR2_PRG)) return false;
        for (int b = b0; b <= b1; b++) {
            const uint8_t *blk = img->flash + (uint32_t)b * STM8_FLASH_BLOCK;
            bool all_ff = true;
            for (int i = 0; i < STM8_FLASH_BLOCK; i++)
                if (blk[i] != 0xFF) { all_ff = false; break; }
            if (!all_ff) {                       /* 全 0xFF 的块跳过(已擦除) */
                if (!clear_eop()) goto fail;
                if (!swim_wotf(STM8_FLASH_BASE + (uint32_t)b * STM8_FLASH_BLOCK,
                               blk, STM8_FLASH_BLOCK)) goto fail;
                if (!wait_eop()) goto fail;
            }
            if (cb) cb((b - b0 + 1) * 100 / (b1 - b0 + 1));
        }
        swim_write8(R_CR2, 0);
    }

    /* --- EEPROM: 字编程 (无需擦除) --- */
    if (img->eep_used) {
        if (!swim_write8(R_CR2, CR2_WPRG)) return false;
        for (int w = 0; w < STM8_EEP_SIZE / 4; w++) {
            if (!clear_eop()) goto fail;
            if (!swim_wotf(STM8_EEP_BASE + (uint32_t)w * 4, img->eep + w * 4, 4)) goto fail;
            if (!wait_eop()) goto fail;
        }
        swim_write8(R_CR2, 0);
    }
    return true;
fail:
    swim_write8(R_CR2, 0);
    return false;
}

bool stm8_verify(const image_t *img, uint32_t *fail_addr, prog_cb_t cb) {
    uint8_t buf[128];
    uint32_t lo = img->flash_lo, hi = img->flash_hi;
    if (hi > lo) {
        for (uint32_t a = lo; a < hi; a += sizeof buf) {
            uint32_t n = hi - a; if (n > sizeof buf) n = sizeof buf;
            if (!swim_rotf(a, buf, n)) return false;
            for (uint32_t i = 0; i < n; i++)
                if (buf[i] != img->flash[a - STM8_FLASH_BASE + i]) {
                    *fail_addr = a + i;
                    return false;
                }
            if (cb) cb((int)((a - lo + n) * 100 / (hi - lo)));
        }
    }
    if (img->eep_used) {
        if (!swim_rotf(STM8_EEP_BASE, buf, STM8_EEP_SIZE)) return false;
        for (int i = 0; i < STM8_EEP_SIZE; i++)
            if (buf[i] != img->eep[i]) { *fail_addr = STM8_EEP_BASE + (uint32_t)i; return false; }
    }
    return true;
}

bool stm8_readout(image_t *img, prog_cb_t cb) {
    image_init(img);
    for (uint32_t a = 0; a < STM8_FLASH_SIZE; a += 128) {
        if (!swim_rotf(STM8_FLASH_BASE + a, img->flash + a, 128)) return false;
        if (cb) cb((int)((a + 128) * 90 / STM8_FLASH_SIZE));
    }
    img->flash_lo = STM8_FLASH_BASE;
    img->flash_hi = STM8_FLASH_BASE + STM8_FLASH_SIZE;
    if (!swim_rotf(STM8_EEP_BASE, img->eep, STM8_EEP_SIZE)) return false;
    img->eep_used = true;
    if (cb) cb(100);
    return true;
}