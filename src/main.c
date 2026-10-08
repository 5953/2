#include "pico/stdlib.h"
#include "tusb.h"
#include "board_config.h"
#include "oled.h"
#include "keys.h"
#include "storage.h"
#include "files.h"
#include "ui.h"
#include "swim.h"

int main(void) {
    oled_init();
    keys_init();

    /* 隐藏自检: 上电时同时按住 UP+DOWN, 进入 SWIM 波形测试模式
     * (此时不需要接目标板; 示波器探头接 GP2/GND 量波形; 按复位键退出) */
    if (!gpio_get(PIN_KEY_UP) && !gpio_get(PIN_KEY_DOWN)) {
        oled_clear();
        oled_text_center(20, "SWIM TEST MODE");
        oled_text_center(34, "SCOPE ON GP2");
        oled_flush();
        swim_testmode();    /* 永不返回 */
    }

    storage_init();
    files_init();        /* 首次上电自动格式化 U 盘区 */
    tusb_init();
    ui_init();

    while (1) {
        tud_task();
        key_t k = keys_poll();
        if (k != KEY_NONE) ui_key(k);
        ui_tick();
        storage_idle_flush();
    }
}