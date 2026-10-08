#include "files.h"
#include "storage.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static FATFS   fs;
static uint8_t mkwork[4096];
static uint8_t filebuf[48 * 1024];      /* 固件文件读取缓冲 */

/* 每次文件操作前重挂载, 保证和电脑端看到的 FAT 一致 */
static void remount(void) {
    storage_sync();
    f_mount(NULL, "", 1);
    f_mount(&fs, "", 1);
}

void files_init(void) {
    if (f_mount(&fs, "", 1) != FR_OK) {          /* 空片: 格式化为 FAT */
        MKFS_PARM opt = { FM_FAT, 0, 0, 0, 0 };
        if (f_mkfs("", &opt, mkwork, sizeof mkwork) == FR_OK) {
            f_setlabel("STM8PROG");
            f_mount(&fs, "", 1);
        }
    }
}

static bool has_fw_ext(const char *n, bool *is_bin) {
    const char *dot = strrchr(n, '.');
    if (!dot) return false;
    char e[5] = {0};
    int i = 0;
    for (const char *p = dot + 1; *p && i < 4; p++) e[i++] = (char)tolower((unsigned char)*p);
    if (!strcmp(e, "hex") || !strcmp(e, "ihx")) { *is_bin = false; return true; }
    if (!strcmp(e, "bin"))                      { *is_bin = true;  return true; }
    return false;
}

int files_scan(fileent_t *out, int maxn) {
    remount();
    DIR dir;
    FILINFO fi;
    int n = 0;
    if (f_opendir(&dir, "/") != FR_OK) return 0;
    while (n < maxn && f_readdir(&dir, &fi) == FR_OK && fi.fname[0]) {
        if (fi.fattrib & (AM_DIR | AM_HID | AM_SYS)) continue;   /* 跳过系统垃圾 */
        const char *name = fi.fname;
        bool is_bin;
        if (!has_fw_ext(name, &is_bin)) continue;
        snprintf(out[n].name, FILE_NAME_MAX, "%s", name);
        for (char *p = out[n].name; *p; p++) *p = (char)toupper((unsigned char)*p);
        out[n].size   = (uint32_t)fi.fsize;
        out[n].is_bin = is_bin;
        n++;
    }
    f_closedir(&dir);
    for (int i = 1; i < n; i++)                                  /* 按名字排序 */
        for (int j = i; j > 0 && strcmp(out[j-1].name, out[j].name) > 0; j--) {
            fileent_t t = out[j-1]; out[j-1] = out[j]; out[j] = t;
        }
    return n;
}

bool files_load(const char *name, image_t *img, char *err, int errlen) {
    remount();
    FIL f;
    if (f_open(&f, name, FA_READ) != FR_OK) {
        snprintf(err, errlen, "OPEN FAIL");
        return false;
    }
    UINT sz = f_size(&f);
    if (sz > sizeof filebuf) {
        f_close(&f);
        snprintf(err, errlen, "FILE TOO BIG");
        return false;
    }
    UINT br = 0;
    FRESULT r = f_read(&f, filebuf, sz, &br);
    f_close(&f);
    if (r != FR_OK || br != sz) {
        snprintf(err, errlen, "READ FAIL");
        return false;
    }

    image_init(img);
    bool is_bin;
    has_fw_ext(name, &is_bin);
    if (is_bin) {
        if (sz > STM8_FLASH_SIZE) {
            snprintf(err, errlen, "BIN > 8KB");
            return false;
        }
        image_add(img, STM8_FLASH_BASE, filebuf, sz);
    } else {
        if (!hex_parse_mem(img, (const char *)filebuf, sz, err, errlen)) return false;
    }
    if (img->flash_hi <= img->flash_lo && !img->eep_used) {
        snprintf(err, errlen, "EMPTY IMAGE");
        return false;
    }
    return true;
}

bool files_save_dump(const image_t *img, char *err, int errlen) {
    remount();
    FIL f;
    UINT bw;
    if (f_open(&f, "FLASH.BIN", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK ||
        f_write(&f, img->flash, STM8_FLASH_SIZE, &bw) != FR_OK || bw != STM8_FLASH_SIZE) {
        f_close(&f);
        snprintf(err, errlen, "WRITE FLASH.BIN");
        return false;
    }
    f_close(&f);
    if (f_open(&f, "EEPROM.BIN", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK ||
        f_write(&f, img->eep, STM8_EEP_SIZE, &bw) != FR_OK || bw != STM8_EEP_SIZE) {
        f_close(&f);
        snprintf(err, errlen, "WRITE EEPROM.BIN");
        return false;
    }
    f_close(&f);
    storage_sync();
    storage_notify_change();
    return true;
}

uint32_t files_free_kb(void) {
    remount();
    DWORD free_cl;
    FATFS *pfs;
    if (f_getfree("", &free_cl, &pfs) != FR_OK) return 0;
    return (uint32_t)(free_cl * pfs->csize) / 2;
}
