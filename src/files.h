#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hexparse.h"

#define FILES_MAX     32
#define FILE_NAME_MAX 64

typedef struct {
    char     name[FILE_NAME_MAX];
    uint32_t size;
    bool     is_bin;
} fileent_t;

void     files_init(void);   /* 挂载; 首次使用自动格式化 */
int      files_scan(fileent_t *out, int maxn);
bool     files_load(const char *name, image_t *img, char *err, int errlen);
bool     files_save_dump(const image_t *img, char *err, int errlen);
uint32_t files_free_kb(void);