# STM8 SWIM 烧录器 (RP2040-Zero)

基于 RP2040-Zero 的 STM8S003 离线烧录器: 电脑把固件拷进 U 盘,
OLED 菜单选择文件, 按键一键烧录/校验/读取/擦除.

## 接线

| RP2040-Zero | 接哪里 | 说明 |
|---|---|---|
| 3V3 | 目标板 VCC + OLED VCC | 由烧录器给目标板供电 (只能 3.3V!) |
| GND | 目标板 GND + OLED GND + 按键公共端 | |
| GP2 | 目标板 SWIM (PD1) | 串 220Ω, 4.7kΩ 上拉到 3.3V |
| GP4 | 目标板 NRST | 10kΩ 上拉到 3.3V |
| GP28 | OLED SDA | SSD1315, 地址 0x3C |
| GP29 | OLED SCL | |
| GP14/15/26/27 | K1/K2/K3/K4 | 另一端接地 (上/下/确定/返回) |

!! 注意: RP2040 IO 不耐 5V, 目标板必须用 3.3V 供电烧录 !!

## 构建

### GitHub Actions (推荐)
push 代码即可, 构建产物 (Artifacts) 里下载 `stm8prog.uf2`。

### 本地
```bash
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi
git clone --depth 1 --branch 2.1.1 https://github.com/raspberrypi/pico-sdk.git
(cd pico-sdk && git submodule update --init lib/tinyusb)
export PICO_SDK_PATH=$PWD/pico-sdk
mkdir -p lib/fatfs
curl -L -o /tmp/ff.zip https://elm-chan.org/fsw/ff/arc/ff15.zip
unzip -o /tmp/ff.zip -d /tmp/ff && cp /tmp/ff/source/*.c /tmp/ff/source/*.h lib/fatfs/
cmake -B build && cmake --build build -j
```

## 使用
1. 按住 BOOT 插 USB, 拷入 stm8prog.uf2
2. 首次上电自动格式化 U 盘区 (1MB, 卷标 STM8PROG)
3. 电脑拷入 .hex / .bin 固件, **弹出 U 盘** 后操作菜单
4. FLASH FIRMWARE → 选文件 → OK → 自动 擦除→烧录→校验
5. READ CHIP 会把芯片内容存成 FLASH.BIN / EEPROM.BIN
   (读完需在电脑上重新插拔或刷新才能看到新文件)

## 故障排查 (SWIM 时序)

连不上芯片时, 大概率是时序参数需要微调。用逻辑分析仪抓 SWIM 脚:
- 进入序列: NRST 低期间 4×1kHz + 4×2kHz 脉冲
- 位周期 ~10us: '1' = 低 1us + 高 9us, '0' = 低 8us + 高 2us
- 帧: 起始位(0) + 8 数据位(MSB先) + 偶校验位 + ACK 位
修改 `src/swim.c` 顶部的 T_ 参数后重新编译。
参考资料: ST 文档 UM0470 (SWIM 协议), PM0051 (flash 编程).

- UNLOCK FAIL / ROP PROTECTED?: 芯片被读保护, 需先解除 ROP
  (当前版本未实现解除功能, 解除会清空芯片)
- 目标芯片里跑过的程序如果开了看门狗, 可能干扰烧录, 重试即可

## 已知限制 / 后续计划
- 暂不烧写 option bytes (hex 里 0x4800 区域会被忽略)
- 暂不支持 .s19 格式
- STM32 (SWD) 支持预留中: 需再加一根 SWCLK 线 (建议 GP3)