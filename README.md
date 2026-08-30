# Raspberry Pi Pico W — API 與 Bare-Metal 練習整理

此 Repository 用來整理 Raspberry Pi Pico W / RP2040 的 GPIO、I²C、UART 與 BH1750 感測器實作。

目前同一類功能分成兩種方式：

| 類型 | 說明 |
|---|---|
| Pico SDK API | 使用 Pico SDK 提供的函式完成初始化與資料傳輸 |
| Bare-Metal / Register-Level | 直接依照 RP2040 Datasheet 操作硬體 Register |
| 補充 | Bare-Metal 程式目前仍保留 `stdio_init_all()`、`sleep_ms()`、`clock_get_hz()` 等少量 Pico SDK 輔助函式 |

---

## 專案檔案

| 檔案 | 分類 | 主要用途 |
|---|---|---|
| `bh1750_api.c` | Pico SDK API | 使用 Pico SDK I²C API 讀取 BH1750 照度 |
| `bh1750_baremetal.c` | Bare-Metal / Register-Level | 直接操作 I²C0、GPIO、PADS 等 Register 讀取 BH1750 |
| `uart_api.c` | Pico SDK API | 使用 Pico SDK UART API 初始化 UART0 並傳送資料 |
| `uart_baremetal.c` | Bare-Metal / Register-Level | 直接操作 UART0 Register，設定 Baud Rate、8N1、FIFO 並傳送資料 |
| `bh1750_uart_baremetal.c` | Bare-Metal / Register-Level | 整合 BH1750 I²C 讀取與 UART TX 傳送 |
| `gpio_voltage_test.c` | Pico SDK API | 將 GP2 設為 Output High，方便做 GPIO / 電壓量測 |
| `CMakeLists.txt` | Build 設定 | 指定目前編譯的 `.c` 檔並連結 Pico SDK / I²C / UART Library |
| `pico_sdk_import.cmake` | Pico SDK 設定 | 匯入 Pico SDK |
| `.vscode/` | VS Code 設定 | 儲存 VS Code / Pico Extension 專案設定 |
| `build/` | 編譯輸出 | CMake / Ninja 編譯後產生的 `.uf2`、`.elf` 等檔案 |
| `.gitignore` | Git 設定 | 指定不需要提交到 GitHub 的檔案或目錄 |

---

## BH1750 API 版本流程

| 順序 | 動作 | 使用內容 |
|---:|---|---|
| 1 | 初始化 USB / stdio | `stdio_init_all()` |
| 2 | 初始化 I²C0 | `i2c_init(i2c0, 100000)` |
| 3 | 設定 GP4 為 SDA | `gpio_set_function()` |
| 4 | 設定 GP5 為 SCL | `gpio_set_function()` |
| 5 | 開啟 SDA / SCL Pull-Up | `gpio_pull_up()` |
| 6 | 設定 BH1750 Address | `0x23` |
| 7 | 傳送 Continuous High Resolution 指令 | `0x10` |
| 8 | 等待第一次量測 | `sleep_ms(180)` |
| 9 | 讀取 2 Byte | `i2c_read_blocking()` |
| 10 | 合併 High / Low Byte | `(high << 8) \| low` |
| 11 | 換算 Lux | `raw / 1.2f` |
| 12 | 輸出結果 | `printf()` |

---

## BH1750 Bare-Metal 版本流程

| 順序 | 動作 | 主要 Register / 位址 |
|---:|---|---|
| 1 | Reset I²C0 | `RESETS` |
| 2 | 等待 I²C0 Reset Done | `RESET_DONE` |
| 3 | 設定 GP4 為 I²C0 SDA | `GPIO4_CTRL` |
| 4 | 設定 GP4 Pull-Up | `PADS_BANK0 GPIO4` |
| 5 | 設定 GP5 為 I²C0 SCL | `GPIO5_CTRL` |
| 6 | 設定 GP5 Pull-Up | `PADS_BANK0 GPIO5` |
| 7 | Disable I²C0 | `IC_ENABLE` |
| 8 | 設定 SCL High Count | `IC_SS_SCL_HCNT = 642` |
| 9 | 設定 SCL Low Count | `IC_SS_SCL_LCNT = 587` |
| 10 | 設定 BH1750 Address | `IC_TAR = 0x23` |
| 11 | 設定 I²C Master 模式 | `IC_CON = 0x63` |
| 12 | Enable I²C0 | `IC_ENABLE` |
| 13 | 傳送 `0x10` | `IC_DATA_CMD` |
| 14 | 等待量測完成 | `sleep_ms(180)` |
| 15 | 發出 High Byte Read | `IC_DATA_CMD = 0x100` |
| 16 | 發出 Low Byte Read + STOP | `IC_DATA_CMD = 0x300` |
| 17 | 等待 RX FIFO 有資料 | `IC_RXFLR` |
| 18 | 讀取 High Byte | `IC_DATA_CMD & 0xFF` |
| 19 | 讀取 Low Byte | `IC_DATA_CMD & 0xFF` |
| 20 | 合併資料 | `(high << 8) \| low` |
| 21 | 換算 Lux | `raw / 1.2f` |
| 22 | 輸出結果 | `printf()` |

---

## UART API 版本流程

| 順序 | 動作 | 使用內容 |
|---:|---|---|
| 1 | 初始化 stdio | `stdio_init_all()` |
| 2 | 設定 UART TX Pin | `gpio_set_function()` |
| 3 | 設定 UART RX Pin | `gpio_set_function()` |
| 4 | 初始化 UART0 | `uart_init()` |
| 5 | 設定 Baud Rate | `115200` |
| 6 | 關閉 CTS / RTS | `uart_set_hw_flow()` |
| 7 | 設定資料格式 | `8N1` |
| 8 | 傳送資料 | `uart_puts()` |
| 9 | USB 同步除錯輸出 | `printf()` |

---

## UART Bare-Metal 版本流程

| 順序 | 動作 | 主要 Register / 功能 |
|---:|---|---|
| 1 | Reset UART0 | `RESETS` bit 22 |
| 2 | 等待 UART0 Reset Done | `RESET_DONE` |
| 3 | 設定 UART TX Pin | `GPIOx_CTRL` |
| 4 | 設定 UART RX Pin | `GPIOx_CTRL` |
| 5 | 視需要設定 RX Pull-Up | `PADS_BANK0` |
| 6 | 關閉 UART | `UARTCR` |
| 7 | 設定 Baud Rate 整數部分 | `UARTIBRD = 67` |
| 8 | 設定 Baud Rate 小數部分 | `UARTFBRD = 52` |
| 9 | 設定資料格式 | `UARTLCR_H = 0x70` |
| 10 | 啟用 UART / TX / RX | `UARTCR` |
| 11 | 檢查 TX FIFO 是否滿 | `UARTFR` 的 `TXFF` |
| 12 | 將字元寫入 TX | `UARTDR` |
| 13 | 重複直到整段字串送完 | Polling TX |

---

## BH1750 + UART Bare-Metal 整合流程

| 順序 | 資料流程 | 說明 |
|---:|---|---|
| 1 | BH1750 → I²C0 | Pico W 透過 I²C0 讀取 BH1750 |
| 2 | I²C0 → High / Low Byte | 從 RX FIFO 取得兩個 Byte |
| 3 | High / Low Byte → Raw | 合併成 16-bit 原始值 |
| 4 | Raw → Lux | 使用 `raw / 1.2f` 計算照度 |
| 5 | Lux → 字串 | 使用 `snprintf()` 組成 `LUX=xx.xx` |
| 6 | 字串 → UARTDR | 每次送出 1 Byte |
| 7 | UART0 TX → 外部裝置 | 將照度資料透過 UART 傳送出去 |
| 8 | 延遲 | `sleep_ms(500)` |
| 9 | 重複讀取 | 回到 BH1750 讀取流程 |

---

## GPIO 電壓測試流程

| 順序 | 動作 | 使用內容 |
|---:|---|---|
| 1 | 初始化 GP2 | `gpio_init(2)` |
| 2 | 設定為 Output | `gpio_set_dir(2, GPIO_OUT)` |
| 3 | 輸出 High | `gpio_put(2, 1)` |
| 4 | 保持程式執行 | `while(true)` |
| 5 | 外部量測 | 使用三用電錶量測 GP2 電壓 |

---

## API 與 Bare-Metal 對照

| 功能 | Pico SDK API | Bare-Metal / Register-Level |
|---|---|---|
| GPIO Pin Mux | `gpio_set_function()` | `GPIOx_CTRL` |
| GPIO Pull-Up | `gpio_pull_up()` | `PADS_BANK0` |
| I²C 初始化 | `i2c_init()` | `IC_CON`、`IC_ENABLE`、HCNT / LCNT |
| I²C 寫入 | `i2c_write_blocking()` | `IC_DATA_CMD` |
| I²C 讀取 | `i2c_read_blocking()` | `IC_DATA_CMD` + `IC_RXFLR` |
| UART 初始化 | `uart_init()` | `UARTCR` + Baud Rate Registers |
| UART 資料格式 | `uart_set_format()` | `UARTLCR_H` |
| UART 傳送 | `uart_puts()` | `UARTFR` + `UARTDR` |
| GPIO Output | `gpio_put()` | GPIO / SIO Register |

---

## CMake 編譯選擇

| 狀態 | CMake 設定 |
|---|---|
| BH1750 API | `#add_executable(260818 bh1750_api.c)` |
| BH1750 Bare-Metal | `#add_executable(260818 bh1750_baremetal.c)` |
| UART API | `#add_executable(260818 uart_api.c)` |
| UART Bare-Metal | `#add_executable(260818 uart_baremetal.c)` |
| BH1750 + UART Bare-Metal | `add_executable(260818 bh1750_uart_baremetal.c)` |
| GPIO Voltage Test | `#add_executable(260818 gpio_voltage_test.c)` |

---

## CMake Target

| 項目 | 目前設定 |
|---|---|
| Target 名稱 | `260818` |
| Project 名稱 | `260818` |
| Program 名稱 | `260818` |
| Pico Board | `pico_w` |
| C Standard | C11 |
| Pico SDK | 2.3.0 |
| USB stdio | Enable |
| UART stdio | Disable |
| Linked Libraries | `pico_stdlib`、`hardware_i2c`、`hardware_uart` |

---

## Repository 學習主軸

| 階段 | 目的 | 實作方式 |
|---:|---|---|
| 1 | 先確認硬體與感測器功能可以正常運作 | Pico SDK API |
| 2 | 理解 GPIO、I²C、UART 底層控制流程 | Datasheet |
| 3 | 找出 Peripheral Base Address 與 Register Offset | Register Mapping |
| 4 | 直接讀寫 RP2040 Register | `volatile uint32_t *` |
| 5 | 將 I²C 與 UART Bare-Metal 實作整合 | BH1750 → Pico W → UART |
| 6 | 對照 API 與 Register-Level 差異 | 學習 Driver / Hardware Control 基礎 |
