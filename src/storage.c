/* 内部 flash 的后 1MB 模拟成 512B 扇区磁盘.
 * flash 擦除粒度 4KB, 用一个 4KB 缓存做读-改-写.
 * MSC(电脑) 和 FatFs(本机) 都走这里, 保证数据一致. */
#include "storage.h"
#include "board_config.h"
#include "hardware/flash.h"
#include "hardware/address_mapped.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <string.h>

#define FSEC_SIZE  4096

static uint8_t  cache[FSEC_SIZE];
static int      cached = -1;      /* 当前缓存的 4KB 扇区号 */
static bool     dirty;
static uint32_t last_wr;
static uint32_t generation;

static const uint8_t *xip(uint32_t addr) {   /* addr: 存储区内字节偏移 */
    return (const uint8_t *)(XIP_BASE + STORAGE_OFFSET + addr);
}

static void flush(void) {
    if (!dirty) return;
    uint32_t off = STORAGE_OFFSET + (uint32_t)cached * FSEC_SIZE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(off, FSEC_SIZE);
    flash_range_program(off, cache, FSEC_SIZE);
    restore_interrupts(ints);
    dirty = false;
}

void storage_init(void) { }

uint32_t storage_read(uint32_t lba, uint32_t offset, void *buf, uint32_t size) {
    uint32_t addr = lba * 512 + offset;
    uint32_t done = size;
    uint8_t *out = buf;
    while (size) {
        int      fs = (int)(addr / FSEC_SIZE);
        uint32_t o  = addr % FSEC_SIZE;
        uint32_t n  = FSEC_SIZE - o; if (n > size) n = size;
        const uint8_t *src = (fs == cached) ? cache + o : xip(addr);
        memcpy(out, src, n);
        out += n; addr += n; size -= n;
    }
    return done;
}

uint32_t storage_write(uint32_t lba, uint32_t offset, const uint8_t *buf, uint32_t size) {
    uint32_t addr = lba * 512 + offset;
    uint32_t done = size;
    while (size) {
        int      fs = (int)(addr / FSEC_SIZE);
        uint32_t o  = addr % FSEC_SIZE;
        uint32_t n  = FSEC_SIZE - o; if (n > size) n = size;
        if (fs != cached) {
            flush();
            memcpy(cache, xip((uint32_t)fs * FSEC_SIZE), FSEC_SIZE);
            cached = fs;
        }
        memcpy(cache + o, buf, n);
        dirty = true;
        last_wr = to_ms_since_boot(get_absolute_time());
        buf += n; addr += n; size -= n;
    }
    generation++;
    return done;
}

void storage_sync(void) { flush(); }

void storage_idle_flush(void) {
    if (dirty && to_ms_since_boot(get_absolute_time()) - last_wr > 300) flush();
}

void     storage_notify_change(void) { generation++; }
uint32_t storage_generation(void)    { return generation; }