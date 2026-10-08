#pragma once

/* OLED SSD1315 (I2C0) */
#define PIN_OLED_SDA   28
#define PIN_OLED_SCL   29
#define OLED_I2C       i2c0
#define OLED_ADDR      0x3C        /* 若不亮改成 0x3D */

/* 按键 (另一端接地, 低电平有效) */
#define PIN_KEY_UP     14          /* K1 */
#define PIN_KEY_DOWN   15          /* K2 */
#define PIN_KEY_OK     26          /* K3 */
#define PIN_KEY_BACK   27          /* K4 */

/* 目标机接口 */
#define PIN_SWIM       2
#define PIN_NRST       4

/* 存储区: 2MB flash 的后 1MB 作为 U 盘 */
#define STORAGE_OFFSET   (1024*1024)
#define STORAGE_SIZE     (1024*1024)
#define STORAGE_SECTORS  (STORAGE_SIZE/512)

/* STM8S003F3P6 */
#define STM8_FLASH_BASE  0x8000
#define STM8_FLASH_SIZE  0x2000    /* 8KB */
#define STM8_FLASH_BLOCK 128
#define STM8_EEP_BASE    0x4000
#define STM8_EEP_SIZE    128

#define FW_VERSION "1.0"