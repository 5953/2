/* FatFs diskio 实现: 转到 storage 层 */
#include "ff.h"
#include "diskio.h"
#include "storage.h"
#include "board_config.h"

DSTATUS disk_initialize(BYTE pdrv) { (void)pdrv; return 0; }
DSTATUS disk_status(BYTE pdrv)     { (void)pdrv; return 0; }

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    storage_read(sector, 0, buff, count * 512u);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    storage_write(sector, 0, buff, count * 512u);
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    (void)pdrv;
    switch (cmd) {
    case CTRL_SYNC:        storage_sync();                    return RES_OK;
    case GET_SECTOR_COUNT: *(LBA_t *)buff = STORAGE_SECTORS;  return RES_OK;
    case GET_SECTOR_SIZE:  *(WORD  *)buff = 512;              return RES_OK;
    case GET_BLOCK_SIZE:   *(DWORD *)buff = 8;                return RES_OK; /* 4KB */
    }
    return RES_PARERR;
}
DWORD get_fattime(void) {
    return 0;   /* 时间未知, FatFs 接受 */
}
