/* 菜单/界面状态机 (中文版) */
#include "ui.h"
#include "oled.h"
#include "files.h"
#include "stm8.h"
#include "board_config.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

typedef enum { ST_SPLASH, ST_MENU, ST_FILES, ST_CONFIRM, ST_RESULT, ST_ABOUT } state_t;
typedef enum { OP_NONE, OP_FLASH, OP_VERIFY, OP_ERASE } op_t;

static state_t   state;
static op_t      pending_op;
static int       menu_sel, menu_top;
static int       file_sel, file_top;
static int       file_count;
static fileent_t files[FILES_MAX];
static image_t   img;                     /* 8KB+, 必须 static */
static uint32_t  splash_end;

#define MENU_COUNT 5
static const char *menu_items[MENU_COUNT] = {
    "烧录固件", "校验固件", "读取芯片", "整片擦除", "关于",
};

static const char *cur_stage;

static void on_prog(int pct) {            /* 长操作期间保持 USB 活着 + 刷进度 */
    tud_task();
    oled_progress(pct, cur_stage);
}

/* 16px 反白中文标题条 */
static void title_bar(const char *s) {
    oled_rect(0, 0, 128, 16, true);
    oled_text_zh_inv((128 - oled_text_zh_width(s)) / 2, 0, s);
}

static void set_result(bool ok, const char *zh, const char *detail) {
    (void)ok;
    state = ST_RESULT;
    oled_clear();
    oled_text_zh_center(20, zh);
    if (detail) oled_text_center(44, detail);
    oled_text_center(56, "PRESS ANY KEY");
    oled_flush();
}

/* ---------- 烧录/校验/读取/擦除 ---------- */

static void do_flash(void) {
    char err[24];
    cur_stage = "读取文件"; on_prog(0);
    if (!files_load(files[file_sel].name, &img, err, sizeof err)) { set_result(false, "读取文件失败", err); return; }

    cur_stage = "连接中"; on_prog(0);
    if (!stm8_connect()) { set_result(false, "连接失败", "CHECK WIRING"); return; }
    if (!stm8_unlock())  { stm8_disconnect_run(); set_result(false, "芯片保护", "ROP LOCKED?"); return; }

    cur_stage = "擦除中";
    if (!stm8_erase_flash(on_prog)) { stm8_disconnect_run(); set_result(false, "擦除失败", 0); return; }

    cur_stage = "烧录中";
    if (!stm8_program(&img, on_prog)) { stm8_disconnect_run(); set_result(false, "烧录失败", 0); return; }

    cur_stage = "校验中";
    uint32_t bad;
    if (!stm8_verify(&img, &bad, on_prog)) {
        char a[24];
        snprintf(a, sizeof a, "DIFF AT %04lX", (unsigned long)bad);
        stm8_disconnect_run();
        set_result(false, "校验失败", a);
        return;
    }

    stm8_lock();
    stm8_disconnect_run();
    set_result(true, "烧录成功", 0);
}

static void do_verify(void) {
    char err[24];
    cur_stage = "读取文件"; on_prog(0);
    if (!files_load(files[file_sel].name, &img, err, sizeof err)) { set_result(false, "读取文件失败", err); return; }

    cur_stage = "连接中"; on_prog(0);
    if (!stm8_connect()) { set_result(false, "连接失败", "CHECK WIRING"); return; }

    cur_stage = "校验中";
    uint32_t bad;
    bool ok = stm8_verify(&img, &bad, on_prog);
    stm8_disconnect_run();
    if (ok) set_result(true, "校验成功", 0);
    else {
        char a[24];
        snprintf(a, sizeof a, "DIFF AT %04lX", (unsigned long)bad);
        set_result(false, "校验失败", a);
    }
}

static void do_read(void) {
    char err[24];
    cur_stage = "连接中"; on_prog(0);
    if (!stm8_connect()) { set_result(false, "连接失败", "CHECK WIRING"); return; }

    cur_stage = "读取中";
    if (!stm8_readout(&img, on_prog)) { stm8_disconnect_run(); set_result(false, "读取失败", 0); return; }
    stm8_disconnect_run();

    if (!files_save_dump(&img, err, sizeof err)) { set_result(false, "读取文件失败", err); return; }
    set_result(true, "读取成功", "FLASH+EEPROM.BIN");
}

static void do_erase(void) {
    cur_stage = "连接中"; on_prog(0);
    if (!stm8_connect()) { set_result(false, "连接失败", "CHECK WIRING"); return; }
    if (!stm8_unlock())  { stm8_disconnect_run(); set_result(false, "芯片保护", "ROP LOCKED?"); return; }

    cur_stage = "擦除中";
    bool ok = stm8_erase_flash(on_prog);
    stm8_lock();
    stm8_disconnect_run();
    if (ok) set_result(true, "擦除成功", 0);
    else    set_result(false, "擦除失败", 0);
}

/* ---------- 界面绘制 ---------- */

static void draw_menu(void) {
    oled_clear();
    title_bar("STM8烧录器");
    for (int i = 0; i < 3 && menu_top + i < MENU_COUNT; i++) {   /* 一屏 3 行, 滚动 */
        int idx = menu_top + i;
        int y = 17 + i * 16;
        if (idx == menu_sel) oled_text(2, y + 4, ">");
        oled_text_zh(12, y, menu_items[idx]);
    }
    oled_flush();
}

static void draw_files(void) {
    oled_clear();
    title_bar("选择文件");
    if (file_count == 0) {
        oled_text_zh_center(24, "无文件");
        oled_text_center(48, "COPY VIA USB");
    } else {
        char pg[8];
        snprintf(pg, sizeof pg, "%d/%d", file_sel + 1, file_count);
        oled_text_inv(104, 4, pg);
        for (int i = 0; i < 5 && file_top + i < file_count; i++) {
            int idx = file_top + i;
            int y = 18 + i * 9;
            if (idx == file_sel) oled_text(2, y, ">");
            oled_text(10, y, files[idx].name);
        }
    }
    oled_flush();
}

static void draw_confirm(void) {
    oled_clear();
    title_bar(pending_op == OP_ERASE  ? "确认擦除" :
              pending_op == OP_VERIFY ? "确认校验" : "确认烧录");
    if (pending_op == OP_ERASE) {
        oled_text_zh_center(28, "整片擦除?");
    } else {
        char s[24];
        oled_text(2, 20, files[file_sel].name);
        snprintf(s, sizeof s, "SIZE: %lu B", (unsigned long)files[file_sel].size);
        oled_text(2, 31, s);
        oled_text(2, 42, "TARGET: STM8S003");
    }
    oled_text_zh(4, 48, "OK开始 BACK取消");
    oled_flush();
}

static void draw_about(void) {
    char s[24];
    oled_clear();
    title_bar("关于");
    oled_text(2, 20, "VER " FW_VERSION);
    snprintf(s, sizeof s, "DISK FREE:%luKB", (unsigned long)files_free_kb());
    oled_text(2, 32, s);
    snprintf(s, sizeof s, "FW FILES: %d", file_count);
    oled_text(2, 44, s);
    oled_text(2, 56, "BACK=EXIT");
    oled_flush();
}

/* ---------- 状态机 ---------- */

void ui_key(key_t k) {
    if (k == KEY_NONE) return;
    switch (state) {
    case ST_SPLASH:
        state = ST_MENU; draw_menu(); break;

    case ST_MENU:
        if (k == KEY_UP || k == KEY_DOWN) {
            menu_sel = (k == KEY_UP) ? (menu_sel + MENU_COUNT - 1) % MENU_COUNT
                                     : (menu_sel + 1) % MENU_COUNT;
            if (menu_sel < menu_top)     menu_top = menu_sel;
            if (menu_sel > menu_top + 2) menu_top = menu_sel - 2;
            draw_menu();
        }
        if (k == KEY_OK) {
            if (menu_sel == 0 || menu_sel == 1) {
                pending_op = menu_sel == 0 ? OP_FLASH : OP_VERIFY;
                file_count = files_scan(files, FILES_MAX);
                file_sel = file_top = 0;
                state = ST_FILES; draw_files();
            } else if (menu_sel == 2) {
                do_read();
            } else if (menu_sel == 3) {
                pending_op = OP_ERASE;
                state = ST_CONFIRM; draw_confirm();
            } else {
                file_count = files_scan(files, FILES_MAX);
                state = ST_ABOUT; draw_about();
            }
        }
        break;

    case ST_FILES:
        if (k == KEY_BACK) { state = ST_MENU; draw_menu(); break; }
        if (file_count == 0) break;
        if (k == KEY_UP && file_sel > 0) {
            file_sel--;
            if (file_sel < file_top) file_top = file_sel;
            draw_files();
        }
        if (k == KEY_DOWN && file_sel < file_count - 1) {
            file_sel++;
            if (file_sel >= file_top + 5) file_top = file_sel - 4;
            draw_files();
        }
        if (k == KEY_OK) { state = ST_CONFIRM; draw_confirm(); }
        break;

    case ST_CONFIRM:
        if (k == KEY_BACK) {
            if (pending_op == OP_ERASE) { state = ST_MENU; draw_menu(); }
            else                        { state = ST_FILES; draw_files(); }
            break;
        }
        if (k == KEY_OK) {
            if      (pending_op == OP_FLASH)  do_flash();
            else if (pending_op == OP_VERIFY) do_verify();
            else if (pending_op == OP_ERASE)  do_erase();
        }
        break;

    case ST_RESULT:
    case ST_ABOUT:
        state = ST_MENU; draw_menu(); break;
    }
}

void ui_init(void) {
    state = ST_SPLASH;
    splash_end = to_ms_since_boot(get_absolute_time()) + 1200;
    oled_clear();
    oled_text_zh_center(16, "STM8烧录器");
    oled_text_center(44, "V" FW_VERSION);
    oled_flush();
}

void ui_tick(void) {
    if (state == ST_SPLASH && to_ms_since_boot(get_absolute_time()) > splash_end) {
        state = ST_MENU;
        draw_menu();
    }
}