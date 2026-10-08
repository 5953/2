/* STM8 SWIM 单线协议主机端 (低速模式, 参考 UM0470)
 *
 * 时序: 位周期 ~10us
 *   '1' = 拉低 1us + 释放 9us
 *   '0' = 拉低 8us + 释放 2us
 * 帧:  起始位(0) + 8 数据位(MSB 先) + 偶校验位 + ACK 位(对方拉低 = ACK)
 * 进入序列: NRST 拉低期间, SWIM 发 4 个 1kHz 脉冲 + 4 个 2kHz 脉冲
 *
 * !! 如果连不上芯片, 用示波器对照微调下面的 T_ 参数 !!
 * 自检方法: 上电时同时按住 UP+DOWN, 见 main.c / swim_testmode()
 */
#include "swim.h"
#include "board_config.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"

/* ---- 时序参数 (单位 us) ---- */
#define T_1_LOW     1
#define T_1_HIGH    9
#define T_0_LOW     8
#define T_0_HIGH    2
#define T_ACK_WAIT  25        /* 等 ACK 拉低的最长时间 */
#define T_GAP       10        /* 帧间间隔 */
#define T_RX_START  1000      /* 等目标机发出首帧的超时 */

/* SWIM 脚输出锁存恒为 0, 用方向切换实现开漏: OUT=拉低, IN=释放 */
static inline void swim_low(void) { gpio_set_dir(PIN_SWIM, GPIO_OUT); }
static inline void swim_rel(void) { gpio_set_dir(PIN_SWIM, GPIO_IN);  }

static void pins_init(void) {
    gpio_init(PIN_SWIM);
    gpio_put(PIN_SWIM, 0);
    gpio_set_dir(PIN_SWIM, GPIO_IN);
    gpio_init(PIN_NRST);
    gpio_put(PIN_NRST, 0);
    gpio_set_dir(PIN_NRST, GPIO_IN);
}

static bool wait_level(int level, uint32_t timeout_us) {
    uint32_t t0 = time_us_32();
    while (gpio_get(PIN_SWIM) != level) {
        if (time_us_32() - t0 > timeout_us) return false;
    }
    return true;
}

static void send_bit(int bit) {
    if (bit) { swim_low(); busy_wait_us_32(T_1_LOW); swim_rel(); busy_wait_us_32(T_1_HIGH); }
    else     { swim_low(); busy_wait_us_32(T_0_LOW); swim_rel(); busy_wait_us_32(T_0_HIGH); }
}

/* 发一字节; NACK 自动重发. false = 失败 */
static bool tx_byte(uint8_t b) {
    for (int attempt = 0; attempt < 5; attempt++) {
        uint32_t irq = save_and_disable_interrupts();
        send_bit(0);                          /* 起始位 */
        uint8_t p = 0;
        for (int i = 7; i >= 0; i--) {
            int bit = (b >> i) & 1;
            p ^= (uint8_t)bit;
            send_bit(bit);
        }
        send_bit(p);                          /* 偶校验 */
        swim_rel();
        bool ack = wait_level(0, T_ACK_WAIT);
        if (ack) wait_level(1, T_ACK_WAIT * 2);
        restore_interrupts(irq);
        busy_wait_us_32(T_GAP);
        if (ack) return true;
        busy_wait_us_32(50);
    }
    return false;
}

/* 收一字节: 起始位 + 8 数据 + 校验, 然后主机拉低 ACK */
static bool rx_byte(uint8_t *out) {
    uint32_t irq = save_and_disable_interrupts();
    uint8_t v = 0, p = 0;
    int par = 0;
    bool ok = true;
    uint32_t cell_t0 = 0;

    for (int i = 0; i < 10; i++) {
        if (!wait_level(0, i == 0 ? T_RX_START : 60)) { ok = false; break; }
        cell_t0 = time_us_32();
        if (!wait_level(1, 40)) { ok = false; break; }
        uint32_t w = time_us_32() - cell_t0;  /* 低脉冲宽度 */
        int bit = (w < 5) ? 1 : 0;
        if (i == 0)      { /* 起始位 */ }
        else if (i <= 8) { v = (uint8_t)((v << 1) | bit); p ^= (uint8_t)bit; }
        else             { par = bit; }
    }
    if (ok) {
        while (time_us_32() - cell_t0 < 12) { }   /* 等校验位走完再 ACK */
        swim_low(); busy_wait_us_32(2); swim_rel();
    }
    restore_interrupts(irq);
    busy_wait_us_32(T_GAP);
    if (!ok) return false;
    *out = v;
    return par == p;                              /* 偶校验核对 */
}

/* WOTF (Write On The Fly): 02, N, @H, @L, @E, data[N] */
bool swim_wotf(uint32_t addr, const uint8_t *data, uint32_t len) {
    while (len) {
        uint8_t n = len > 128 ? 128 : (uint8_t)len;
        if (!tx_byte(0x02)) return false;
        if (!tx_byte(n)) return false;
        if (!tx_byte((uint8_t)(addr >> 8)))   return false;
        if (!tx_byte((uint8_t)addr))          return false;
        if (!tx_byte((uint8_t)(addr >> 16)))  return false;
        for (uint8_t i = 0; i < n; i++)
            if (!tx_byte(data[i])) return false;
        data += n; addr += n; len -= n;
    }
    return true;
}

/* ROTF (Read On The Fly): 01, N, @H, @L, @E, 然后收 N 字节 */
bool swim_rotf(uint32_t addr, uint8_t *buf, uint32_t len) {
    while (len) {
        uint8_t n = len > 128 ? 128 : (uint8_t)len;
        if (!tx_byte(0x01)) return false;
        if (!tx_byte(n)) return false;
        if (!tx_byte((uint8_t)(addr >> 8)))   return false;
        if (!tx_byte((uint8_t)addr))          return false;
        if (!tx_byte((uint8_t)(addr >> 16)))  return false;
        for (uint8_t i = 0; i < n; i++)
            if (!rx_byte(&buf[i])) return false;
        buf += n; addr += n; len -= n;
    }
    return true;
}

bool swim_write8(uint32_t addr, uint8_t v) { return swim_wotf(addr, &v, 1); }
bool swim_read8(uint32_t addr, uint8_t *v) { return swim_rotf(addr, v, 1); }
bool swim_srst(void)                         { return tx_byte(0x00); }

static void entry_sequence(void) {
    for (int i = 0; i < 4; i++) { swim_low(); busy_wait_us_32(500); swim_rel(); busy_wait_us_32(500); } /* 1kHz */
    for (int i = 0; i < 4; i++) { swim_low(); busy_wait_us_32(250); swim_rel(); busy_wait_us_32(250); } /* 2kHz */
}

bool swim_connect(void) {
    for (int attempt = 0; attempt < 3; attempt++) {
        pins_init();
        gpio_set_dir(PIN_NRST, GPIO_OUT);   /* 拉住复位 */
        sleep_ms(5);
        entry_sequence();
        gpio_set_dir(PIN_NRST, GPIO_IN);    /* 释放复位 */
        sleep_ms(10);
        swim_srst();
        sleep_ms(10);
        uint8_t v;
        if (swim_read8(STM8_FLASH_BASE, &v)) return true;   /* 链路自检 */
        sleep_ms(50);
    }
    return false;
}

void swim_release_target(void) {
    gpio_set_dir(PIN_SWIM, GPIO_IN);
    gpio_set_dir(PIN_NRST, GPIO_IN);
}

/* 示波器自检: 循环输出 进入序列 + 0x55/0x00 帧, 不接目标板也能量波形
 * 0x55 = 01010101 → 宽窄脉冲交替, 最容易认
 * 0x00 → 全是宽脉冲, 用来核对 '0' 的宽度
 * 无目标机时收不到 ACK, tx_byte 会自动重发 5 次, 顺便可以观察重发行为 */
void swim_testmode(void) {
    pins_init();
    for (;;) {
        gpio_set_dir(PIN_NRST, GPIO_OUT);
        sleep_ms(2);
        entry_sequence();
        gpio_set_dir(PIN_NRST, GPIO_IN);
        sleep_ms(5);
        tx_byte(0x55);
        tx_byte(0x00);
        sleep_ms(20);
    }
}