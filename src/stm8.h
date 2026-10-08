#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hexparse.h"

typedef void (*prog_cb_t)(int pct);

bool stm8_connect(void);
void stm8_disconnect_run(void);        /* 复位目标机 + 释放引脚 */
bool stm8_unlock(void);                /* 解锁 flash+EEPROM; 失败可能是 ROP */
bool stm8_erase_flash(prog_cb_t cb);
bool stm8_program(const image_t *img, prog_cb_t cb);
bool stm8_verify(const image_t *img, uint32_t *fail_addr, prog_cb_t cb);
bool stm8_readout(image_t *img, prog_cb_t cb);
void stm8_lock(void);