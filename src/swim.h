#pragma once
#include <stdint.h>
#include <stdbool.h>

bool swim_connect(void);          /* 进 SWIM 模式并验证链路 */
void swim_release_target(void);   /* 释放引脚, 目标机自由运行 */
bool swim_srst(void);
bool swim_wotf(uint32_t addr, const uint8_t *data, uint32_t len);   /* 写 */
bool swim_rotf(uint32_t addr, uint8_t *buf, uint32_t len);          /* 读 */
bool swim_write8(uint32_t addr, uint8_t v);
bool swim_read8(uint32_t addr, uint8_t *v);

void swim_testmode(void);         /* 示波器自检: 循环出波形, 永不返回 */