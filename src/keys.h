#pragma once
#include <stdint.h>

typedef enum { KEY_NONE = 0, KEY_UP, KEY_DOWN, KEY_OK, KEY_BACK } key_t;

void  keys_init(void);
key_t keys_poll(void);   /* 主循环反复调用; 有事件时返回按键 */