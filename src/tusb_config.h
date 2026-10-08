#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU             OPT_MCU_RP2040
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS   OPT_OS_NONE
#endif
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_DEVICE)

#define CFG_TUSB_DEBUG           0

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN       __attribute__((aligned(4)))
#endif

#define CFG_TUD_ENDPOINT0_SIZE   64

#define CFG_TUD_CDC              1     /* 调试串口(预留) */
#define CFG_TUD_MSC              1     /* U 盘 */
#define CFG_TUD_HID              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0

#define CFG_TUD_CDC_RX_BUFSIZE   256
#define CFG_TUD_CDC_TX_BUFSIZE   512
#define CFG_TUD_MSC_EP_BUFSIZE   512

#ifdef __cplusplus
}
#endif
