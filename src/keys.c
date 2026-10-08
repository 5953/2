/* 4 按键: 低电平有效, 25ms 消抖, UP/DOWN 长按自动连发 */
#include "keys.h"
#include "board_config.h"
#include "pico/stdlib.h"

static const uint8_t pins[4] = { PIN_KEY_UP, PIN_KEY_DOWN, PIN_KEY_OK, PIN_KEY_BACK };

static uint8_t  raw, stable;            /* bit=1 表示按下 */
static uint32_t t_change, t_press, t_rep;

void keys_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
}

static uint8_t read_mask(void) {
    uint8_t m = 0;
    for (int i = 0; i < 4; i++)
        if (!gpio_get(pins[i])) m |= (uint8_t)(1 << i);
    return m;
}

key_t keys_poll(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint8_t m = read_mask();

    if (m != raw) { raw = m; t_change = now; }

    if (m != stable && now - t_change > 25) {     /* 消抖后状态稳定 */
        uint8_t pressed = m & (uint8_t)~stable;   /* 新按下的键 */
        stable = m;
        t_press = now;
        t_rep = now;
        for (int i = 0; i < 4; i++)
            if (pressed & (1 << i)) return (key_t)(i + 1);
    }

    if (stable & 0x03) {                          /* UP/DOWN 连发 */
        if (now - t_press > 550 && now - t_rep > 130) {
            t_rep = now;
            return (stable & 0x01) ? KEY_UP : KEY_DOWN;
        }
    }
    return KEY_NONE;
}