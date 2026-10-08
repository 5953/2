#pragma once
#include <stdint.h>

void     storage_init(void);
uint32_t storage_read (uint32_t lba, uint32_t offset, void *buf, uint32_t size);
uint32_t storage_write(uint32_t lba, uint32_t offset, const uint8_t *buf, uint32_t size);
void     storage_sync(void);          /* 把脏缓存刷进 flash */
void     storage_idle_flush(void);    /* 主循环调用: 写完空闲一会自动刷 */
void     storage_notify_change(void);
uint32_t storage_generation(void);