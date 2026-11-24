# ATlab STM32F407／STM32L476 CAN Bus 韌體

本專案將既有 CAN 草稿整合為兩個可獨立匯入 STM32CubeIDE 的完整工程。F407 每秒送出 HELLO，L476 驗證封包後回 ACK；兩款晶片皆提供 Normal 與內部 Silent Loopback 建置組態。

**本次僅進行軟體開發與電腦端驗證，沒有連接 STM32、燒錄或硬體測試。編譯成功不代表實體 CAN 或 USB 通訊成功。** 本機 CubeIDE 為 2.2.0；找到的 CubeMX 實際為 **6.18.1**，不是要求的 6.19。6.19 的再產生驗證尚未完成。

## 工程與資料夾

```text
CAN_Board_to_Board/
├── ATlab_CAN_F407/          # STM32F407VET6，512 KiB Flash
│   ├── ATlab_CAN_F407.ioc
│   ├── .project / .cproject
│   ├── Core/Inc, Src, Startup
│   ├── Drivers/             # HAL、CMSIS，含原授權檔
│   ├── USB_DEVICE/          # USB CDC interface、PCD、descriptor
│   ├── Middlewares/         # ST USB Device Core + CDC
│   └── STM32F407VETX_*.ld
├── ATlab_CAN_L476/          # STM32L476RGT6，1 MiB Flash
│   ├── ATlab_CAN_L476.ioc
│   ├── .project / .cproject
│   ├── Core/Inc, Src, Startup
│   ├── Drivers/
│   └── STM32L476RGTX_*.ld
├── protocol/                # 封包編解碼、ACK／序號／逾時狀態
├── firmware/                # CAN HAL、主迴圈、logging 佇列與輸出
├── tests/                   # 原生 C 測試及 USB／UART mocks
├── tools/                   # 來源同步、Headless Build、CubeMX CLI
└── reports/                 # 軟體驗證報告、可追溯的文字證據
```

`protocol/` 與 `firmware/` 是應用程式的維護來源。`python tools/sync_sources.py` 將相同檔案複製到兩個工程的 Core；兩個工程不依賴工作目錄外的 HAL、共用程式或絕對路徑連結。修改後必須同步，並由檢查程式驗證副本一致。

## CAN Bus 與應用協定

採 Classical CAN 2.0、CAN1、500 kbit/s、Standard 11-bit ID、DATA frame，未使用 CAN FD。CAN 控制器負責仲裁、CRC、硬體 ACK 與錯誤狀態；`ACK` 應用封包則驗證 L476 是否收到指定 HELLO，兩種 ACK 意義不同。

| 傳送端 | CAN ID | DLC | 內容 |
|---|---|---|---|
| F407 | 0x101 | 6 | ASCII `HELLO` + uint8 sequence |
| L476 | 0x102 | 4 | ASCII `ACK` + 相同 sequence |

F407 每 1000 ms 提交一筆 HELLO；HAL 接受 TX request 後才登記待確認序號。ACK 必須符合 ID、DLC、固定內容及待確認序號，且在 **250 ms 內**收到。重複、過期、錯誤序號或無待確認狀態的 ACK 均拒絕。序號依 0…255 循環；L476 的第一個合法 HELLO 建立基準，後續缺號／重複會計錯，並重新同步，避免遺失一包後永久失步。格式合法但缺號的 HELLO 仍回 ACK，同時保留 sequence_error。

Normal 模式下 L476 不自行送 HELLO。Loopback 模式下兩款 MCU 各自送 HELLO，再由相同韌體回送 ACK，驗證自己的控制器路徑。實際使用 `CAN_MODE_SILENT_LOOPBACK`，不驅動外部 CAN bus；**尚未實際執行 MCU loopback**。

Filter bank 0 使用 32-bit ID list，只接受 0x101／0x102 的 standard DATA frame（IDE、RTR 位元為 0），送入 FIFO0；其他格式在軟體再次檢查。主迴圈每輪最多處理 8 筆 RX，避免持續流量餓死 logging 與逾時計時。

TX 採 one-shot（AutoRetransmission 關閉）；TX 成功計數只在 mailbox complete IRQ 增加，`TX queued` 僅表示提交成功。100 ms 未完成的 mailbox 提出 abort 並計 tx_timeout；不會一直佔用 mailbox。CAN error IRQ 計數，AutoBusOff 開啟。這些 HAL／IRQ／bus-off 行為僅通過編譯與程式碼審查，未在 MCU 驗證。

可在 SWD Expressions 觀察 `atlab_tx_queued`、`atlab_tx_ok`、`atlab_rx_ok`、`atlab_tx_timeout`、`atlab_can_errors`、`atlab_bad_frame`、`atlab_session` 與 logging counters。ISR 只更新計數，不格式化 log。

8-bit 序號不能區分延遲超過整個循環的同序號舊封包；本協定沒有認證、持久化序號或生產級重試機制。此工程供教學與後續接線驗收使用。

## Pin Mapping 與板級限制

兩顆 MCU 的資料表都將 PB8／PB9 的 CAN1 RX／TX 列在 **AF9**，生成的 MSP 使用 `GPIO_AF9_CAN1`。[F407 資料表表 9](https://www.st.com/resource/en/datasheet/stm32f407ve.pdf)、[L476 資料表表 18](https://www.st.com/resource/en/datasheet/stm32l476rg.pdf)。

| 功能 | F407VET6 | NUCLEO-L476RG |
|---|---|---|
| CAN1 RX | PB8 AF9，接收發器 RXD | PB8 AF9，CN5 pin 10 / D15，亦在 CN10 pin 3 |
| CAN1 TX | PB9 AF9，接收發器 TXD | PB9 AF9，CN5 pin 9 / D14，亦在 CN10 pin 5 |
| SWDIO／SWCLK | PA13／PA14；另接 GND、目標電壓參考及可選 NRST | PA13／PA14，既有板載 ST-LINK |
| USB Device DM／DP | PA11／PA12 AF10 | 本工程未啟用 MCU USB |
| USB VBUS sense | PA9；必須依板級電路正確接到 USB VBUS | ST-LINK USB 不等於 MCU USB |
| Logging UART | 未使用 | USART2 PA2 TX／PA3 RX，AF7，115200 8N1 |

NUCLEO-L476RG 的 USB 接頭服務板載 ST-LINK，USART2 經預設 SB13／SB14 連到 ST-LINK VCP。因此本工程使用 **USART2 ST-LINK Virtual COM Port**，不是 L476 的 `CDC_Transmit_FS()`。使用前須確認實板未修改 solder bridge。PB8／PB9 與 I2C D15／D14 共用，驗收時移除使用這兩腳的 shield。板級接腳及 VCP 依 [UM1724 的 NUCLEO-L476RG 表格及 VCP 章節](https://www.st.com/resource/en/user_manual/dm00105823.pdf)。

F407 只提供了晶片型號，沒有確切開發板型號、原理圖及晶振頻率；因此**不能確認實板排針、USB 連接器或 HSE 的相容性**。目前工程以外部 **8 MHz HSE 晶體**為明確組態前提，並使用 PA9 VBUS sensing。若實板是 25 MHz、USB 只供電或只接 USB-UART bridge，須先依實板調整 `.ioc`、HAL HSE_VALUE／PLL 與接線，再重建，不能直接宣稱 CDC 可用。

## 時脈與 CAN Bit Timing

以下由生成的 `SystemClock_Config()`、HAL clock divider 及 `HSE_VALUE` 核對，是工程設定值；不是實板量測。

| 項目 | F407 | L476 |
|---|---|---|
| 時脈來源 | HSE 8 MHz → PLL M=8、N=192、P=2 | HSI 16 MHz，PLL 未啟用 |
| SYSCLK／AHB | 96 MHz／96 MHz | 16 MHz／16 MHz |
| APB1 divider／PCLK1 | /4 → 24 MHz | /1 → 16 MHz |
| CAN prescaler | 3 | 2 |
| Sync／BS1／BS2／SJW | 1／13／2／1 TQ | 1／13／2／1 TQ |
| Bitrate | 24 MHz / (3 × 16) = 500000 | 16 MHz / (2 × 16) = 500000 |
| Sample point | (1+13)/16 = 87.5% | (1+13)/16 = 87.5% |
| USB clock | PLL VCO 192 MHz / Q=4 = 48 MHz | 未啟用 USB，無 48 MHz 消費端 |

`ATLAB_CAN_BringUp()` 執行時讀 `HAL_RCC_GetPCLK1Freq()`，以 `PCLK1 / (500000 × 16)` 計算 prescaler；除不盡或超出 1…1024 就拒絕初始化。CAN 使用 APB1 clock，不能套用 APB timer 的倍頻規則。實體誤差仍受外部晶體／HSI 容差、溫度及匯流排長度影響，將來須量測。

## CDC Transmit 與非阻塞 Logging

F407 具備完整 ST USB Device CDC Core、CDC class、descriptor、PCD 與 OTG_FS IRQ。每秒 heartbeat 包含 sequence、TX complete、RX、ACK、timeout、CAN error、bad packet、log drop；另有 CAN TX queued、CAN RX、ACK OK／INVALID、TIMEOUT 等逐筆訊息。範例訊息格式見程式；本次沒有實板終端輸出紀錄。

`ATLAB_Log()` 將內容複製至 16 筆、每筆最多 191 字元的固定佇列；過長或滿佇列就計 drop，不等待。`ATLAB_Log_Service()` 每輪只嘗試送一筆。USB transport 在短暫 IRQ critical section 中檢查配置狀態、class pointer、DTR 與 TxState，僅空閒時複製至獨立 static TX buffer，再呼叫 `CDC_Transmit_FS()`。這份 buffer 在非同步完成前不會被下一行覆寫；`USBD_BUSY` 保留待送資料，未配置／未開啟／失敗則丟棄舊 log 並計數。USB ISR 的 TxState 歸零／重置後才可重用。正常 API 也加入 class NULL guard。

CDC SET／GET_LINE_CODING 保存並回覆 115200 8N1 格式資料；USB CDC 的 line coding 不決定 USB 實體 bitrate。SET_CONTROL_LINE_STATE 追蹤 DTR，init／deinit 清除狀態。電腦終端需開啟 DTR。即使 host 不讀而讓 TxState 長期忙碌，也只會讓 log 佇列滿、增加 drop，不阻塞 CAN。

L476 使用同一佇列，transport 改為 `HAL_UART_Transmit_IT()`；專用 buffer 一直保留到 UART gState 回到 READY，USART2 IRQ 呼叫 HAL handler。UART 無法偵測電腦是否開啟 VCP；線上位元送完也不證明 PC 已讀到。CDC 非同步狀態的依據可參閱 [ST USB CDC 實作](https://github.com/STMicroelectronics/stm32-mw-usb-device/blob/master/Class/CDC/Src/usbd_cdc.c)。

## 收發器與終端電阻

MCU CAN RX／TX 是邏輯訊號，不能直接接 CAN_H／CAN_L，也不能只交叉接兩塊 MCU 的 RX／TX。每端需要與 3.3 V MCU 邏輯相容的 CAN transceiver：TXD 接 PB9、RXD 接 PB8，兩個收發器的 CAN_H 相連、CAN_L 相連，並依電源與隔離設計建立共同參考地。收發器負責邏輯／差動電壓轉換；需確認 VCC、VIO、standby／enable 與模組原理圖。

總線兩個物理端點各放一顆 CAN_H 與 CAN_L 之間的 **120 Ω** 終端電阻；不要每個節點都加終端。若模組已內建，先確認開關／跳線，避免重複。斷電後量測 H-L 約 60 Ω 是兩顆 120 Ω 並聯的初步檢查，仍須考慮電路其他負載。物理層設計參考 [TI SLOA101](https://www.ti.com/lit/an/sloa101b/sloa101b.pdf)。本次未指定或實測任何收發器模組。

## STM32CubeIDE 匯入及建置

1. 在 CubeIDE 2.2.0 使用 File → Import → Existing Projects into Workspace。
2. 選本資料夾，匯入 `ATlab_CAN_F407` 與 `ATlab_CAN_L476`；不要選舊 UART 工程。
3. Project → Build Configurations → Set Active 選 `Loopback` 或 `Normal`，執行 Clean、Build。
4. `Loopback` 定義 `ATLAB_CAN_TEST_LOOPBACK=1`；`Normal` 定義為 0。不需要改應用程式原始碼。Startup、Flash／RAM linker script、HAL 與必要 middleware 均已包含。後續 SWD 偵錯需手動選正確 ST-LINK、晶片與 ELF；本次沒有啟動硬體 debug session。

自動化命令（PowerShell 7，在 CAN_Board_to_Board 中執行）：

```powershell
python tools/sync_sources.py
./tools/build.ps1
python tests/run_tests.py --cc '你的原生C編譯器路徑'
# 若使用 Zig：
python tests/run_tests.py --cc '你的zig.exe路徑' --zig
# Windows 原生 EXE 被政策阻擋時，可在 PC 執行 WASI 版本：
python tests/run_tests.py --cc '你的zig.exe路徑' --zig --wasm
```

首次 Zig libc 建置可能耗時；可另外取得 [官方 WASI SDK 34.0 sysroot](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34)，在 WASI 命令附加 `--wasi-sysroot 'wasi-sysroot-34.0 的路徑'`。本次五組測試使用此預建 libc，保留 `-Wall -Wextra -Werror -pedantic`、assertions 與 UBSan trap。

WASI 版本使用 [Node.js WASI API](https://nodejs.org/api/wasi.html)，不提供環境變數或 filesystem preopens。此版本驗證可攜式 C 邏輯及 mocks，並不執行 Cortex-M 韌體，也不證明原生 Windows EXE 可啟動。

`build.ps1` 各組態使用獨立 workspace，明確指定 `-cleanBuild project/configuration`，同時檢查日誌、ELF 存在與 SHA256；不把 CLI exit code 0 或「Skipping」當建置成功。產物存入忽略的 Normal／Loopback／build 目錄。

本次 CubeMX CLI 已產生 Core、HAL 與 F407 USB middleware，但內嵌 CubeIDE metadata generator 停滯，未完成完整 project generate。因此 `.project`／`.cproject`、Startup、Linker Script 及 syscalls 依使用者指定的 UART 參考工程 **8a0ba380a44f1942b53ace22eba7dfc87b91d70d** 改名補齊；追加 CDC include/source paths、Normal／Loopback 定義並排除 CMSIS 重複 system template。之後由真正 CubeIDE 2.2.0 Headless Build 驗證。舊 UART 檔案沒有寫入或覆蓋。

`tools/regenerate.ps1 -Chip F407`（或 L476）提供 UTF-8 的 CubeMX `-q` 腳本，保留已審查的 metadata，並設 180 秒上限；這個便利腳本尚未完成成功再產生驗收。需要安裝／確認 CubeMX 6.19 後再產生、審查 diff 與重建。現有工程匯入及建置不需要先重新產生。

## 軟體驗證與限制

實際建置、測試輸出、ELF 大小／SHA256、工具位置及程式碼審查見 [完整開發報告](reports/DEVELOPMENT_REPORT.md) 與 [軟體驗證資料](reports/validation.json)。

| 驗證項目 | 本次狀態 |
|---|---|
| F407 Normal／Loopback Clean Build | 已通過，兩組皆 0 errors／0 warnings |
| L476 Normal／Loopback Clean Build | 已通過，兩組皆 0 errors／0 warnings |
| 電腦端 protocol／session／logging／transport tests | 五組 WASI C tests 全部 PASS；原生 Windows EXE 啟動遭 WinError 4551 阻擋 |
| CDC 佇列及生命週期程式碼審查 | 已完成；mocks 不代表真實 USB 行為 |
| SWD、CAN IRQ、內部 loopback、500 kbps 波形 | 尚未實板驗證 |
| Normal 雙板 HELLO／ACK、電氣與終端 | 尚未實板驗證 |
| F407 USB 枚舉、48 MHz 精度、連線／重連 | 尚未實板驗證；需確認板型、8 MHz HSE 及接線 |
| L476 ST-LINK VCP 實際輸出 | 尚未實板驗證 |
| CubeMX 6.19 再產生 | 尚未驗證；本機找到 6.18.1 |

## 未來硬體接線及驗收（本週不執行）

1. 核對 F407 實板型號、HSE、電源、USB DM／DP／VBUS／接地、SWD 與收發器模組，若與工程假設不同先改 `.ioc` 並重建。核對 NUCLEO revision、SB13／SB14 與 PB8／PB9 沒有衝突。
2. 先燒錄每端 Loopback，個別觀察 heartbeat、TX complete、RX HELLO、TX ACK、ACK OK、序號 255→0。記錄至少 300 次交換及所有 error／timeout；這個步驟才是 MCU 內部 loopback 驗收。
3. F407 以原生 MCU USB 開啟 CDC 終端（DTR on），L476 以 ST-LINK VCP 開 115200 8N1。測試關閉終端、拔 USB、重連、host 暫停讀取；觀察 CAN 節奏持續、drop 計數及緩衝區內容。
4. 斷電接收發器，檢查電源／邏輯電壓／standby、共同參考地、H-H／L-L、兩端 120 Ω，量測終端後才上電。重新以 Normal ELF 燒錄。
5. 用 CAN analyzer／示波器確認 500 kbps、11-bit 0x101／0x102、DLC 6／4、每秒 HELLO、ACK 序號及延遲；至少跨兩次序號循環記錄成功／錯誤／逾時計數。
6. 受控注入錯誤 ID／DLC／payload／sequence、重複／遲到 ACK、暫時停止對端、斷線、恢復及 bus-off；記錄 Filter、計數、abort 與恢復結果。只有完成這些觀察後才能宣稱雙板 CAN 通訊成功。

## 來源與版本追溯

- 原始 CAN 分支起點：`970ff155d320a595897d7a35c801ce1f43c8faa4`。
- [指定 UART 參考工程](https://github.com/ATLab-Rocket/git-intro/tree/8a0ba380a44f1942b53ace22eba7dfc87b91d70d/UART_Board_to_Board)，只讀取來源，未修改該分支。
- [STM32CubeMX 官方頁與 UM1718](https://www.st.com/en/development-tools/stm32cubemx.html)，本機手冊 Rev 50 的 CLI 章節以 `java -jar STM32CubeMX.exe -q script` 生成。
- [ST CAN HAL 原始碼](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_can.c)。實際編入的 driver 以本工程 vendored 檔案及 package 版本為準，master 連結僅供原理查閱。
- [開發分支](https://github.com/LYQjimmy/git-intro/tree/can-f407-l476-software/CAN_Board_to_Board)。不修改 main，不自行 merge。
