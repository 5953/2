/* FatFs R0.15 配置 — 本项目定制
 * 若编译报 "Wrong configuration file (ffconf.h)",
 * 说明 FatFs 版本不对, 把下面的 FFCONF_DEF 改成你那份 ff.c 里要求的值即可. */
#pragma once

#define FFCONF_DEF         80286   /* R0.15 */

/* 功能开关 */
#define FF_FS_READONLY     0
#define FF_FS_MINIMIZE     0
#define FF_USE_FIND        0
#define FF_USE_MKFS        1       /* 需要格式化功能 */
#define FF_USE_FASTSEEK    0
#define FF_USE_EXPAND      0
#define FF_USE_CHMOD       0
#define FF_USE_LABEL       1       /* 需要设置卷标 */
#define FF_USE_FORWARD     0
#define FF_USE_STRFUNC     0
#define FF_PRINT_LLI       0
#define FF_PRINT_FLOAT     0
#define FF_STRF_ENCODE     0
#define FF_FS_CRTIME       0

/* 本地化 */
#define FF_CODE_PAGE       437
#define FF_USE_LFN         1       /* 长文件名, 静态缓冲 */
#define FF_MAX_LFN         255
#define FF_LFN_UNICODE     0
#define FF_LFN_BUF         255
#define FF_SFN_BUF         12
#define FF_FS_RPATH        0

/* 卷/驱动器 */
#define FF_VOLUMES         1
#define FF_STR_VOLUME_ID   0
#define FF_VOLUME_STRS     "D"
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS          512
#define FF_MAX_SS          512
#define FF_LBA64           0
#define FF_USE_TRIM        0

/* 系统 */
#define FF_FS_TINY         0
#define FF_FS_EXFAT        0
#define FF_FS_NORTC        1       /* 无 RTC, 用固定时间戳 */
#define FF_NORTC_MON       1
#define FF_NORTC_MDAY      1
#define FF_NORTC_YEAR      2024
#define FF_FS_NOFSINFO     0
#define FF_FS_LOCK         0
#define FF_FS_REENTRANT    0
#define FF_FS_TIMEOUT      1000
