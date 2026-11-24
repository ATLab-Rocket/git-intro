# ATlab CAN 韌體開發報告

日期：2026-11-24（Asia/Taipei）。本次限定軟體開發，沒有連接 STM32、燒錄、SWD session、實體 USB 或 CAN 測試。

## 交付內容

- `ATlab_CAN_F407`：STM32F407VET6、CAN1 PB8/PB9 AF9、USB Device CDC PA11/PA12、PA9 VBUS sensing、SWD。
- `ATlab_CAN_L476`：STM32L476RGT6、CAN1 PB8/PB9 AF9、USART2 PA2/PA3 ST-LINK VCP、SWD。
- 兩端各有 `.ioc`、`.project`、`.cproject`、Core、Drivers、CMSIS、Startup、Flash/RAM Linker Script；F407 另含完整所需 USB middleware。
- Normal 與 Silent Loopback 由兩個 build configurations 選擇；各端可單獨匯入及編譯。
- 保留原本 HELLO／ACK 格式，新增待確認序號、250 ms 逾時、序號錯誤、TX 提交／完成、RX、CAN error（ISR 與主迴圈以 critical section 保護計數）、invalid frame 及 logging drop 計數。
- 修復 CDC 輸出 buffer 生命週期；固定佇列、專屬 static TX buffer、USBD_BUSY、未配置／NULL class／DTR／離線與重連狀態檢查。
- 所有新檔案及修改均限於 `CAN_Board_to_Board`。未修改 main、原本 UART 檔案或參考 UART 分支，未建立／合併 PR。

## 實際工具位置與版本差異

| 工具 | 本次找到及使用的版本／位置 |
|---|---|
| Windows | Windows 11 環境；本次沒有更改應用程式控制政策 |
| CubeIDE | 2.2.0：`C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE` |
| CubeMX | **6.18.1**：`C:/Users/user/AppData/Local/Programs/STM32CubeMX` |
| ARM GCC | CubeIDE plugin `com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin` |
| F4 package | `C:/Users/user/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3` |
| L4 package | `C:/Users/user/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2` |
| 電腦端 C compiler | 僅在本次工作目錄解壓的 Zig 0.17.1，未修改系統 PATH |
| WASI runtime | 既有 Node.js 24.22.1 |

要求的 **CubeMX 6.19 未在已檢查的本機安裝與下載位置找到**，因此不能將本次結果稱為「6.19 驗證通過」。沒有進行系統升級或代為接受新的 ST 授權。

CubeMX `-q` 已實際執行並產生外設程式、HAL 與 USB middleware；完整 project generate 在內嵌 CubeIDE service 產生 metadata 處停滯。只停止本次 CLI 的程序，沒有停止使用者原本的 IDE。為完成工程，使用指定 UART 參考 Commit `8a0ba380a44f1942b53ace22eba7dfc87b91d70d` 的 metadata、Startup、Linker、syscalls 補齊新 CAN 工程，加入 USB includes/source entries 與模式定義。CDT 的 `fileVersion` processing instruction 及 CMSIS 重複 system source 問題均已修正。工程由 CubeIDE 真正匯入並完成 build；不以 CLI exit code 0 或 Skipping 取代建置證據。

`tools/regenerate.ps1` 是供後續使用的便利腳本，含 UTF-8 路徑與 timeout，**尚未完成成功再產生驗收**。現有工程的匯入／編譯不依賴重新執行它。

## Clean Build 結果

`tools/build.ps1` 已在四個獨立 workspace 實際執行，每組均使用 `-cleanBuild project/configuration`。錯誤已修正後重新 Clean Build，以下為最終結果；完整文字摘要見 [build-summary.txt](build-summary.txt)，每個 ELF 的 SHA256 見 [validation.json](validation.json)。

| MCU／組態 | 結果 | Compiler errors | Compiler warnings | text / data / bss（bytes） |
|---|---|---:|---:|---:|
| F407 Loopback | PASS | 0 | 0 | 37864 / 340 / 12668 |
| F407 Normal | PASS | 0 | 0 | 20244 / 336 / 12672 |
| L476 Loopback | PASS | 0 | 0 | 24456 / 92 / 5660 |
| L476 Normal | PASS | 0 | 0 | 14972 / 92 / 5660 |

ELF 含 debug information 時，檔案大小不等於 Flash 用量。上述大小由 ARM size 讀取；BSS 中亦含 linker 保留區域。兩款晶片的記憶體區域均成功連結。`arm-none-eabi-nm` 已確認四組 ELF 的 CAN TX／RX0／SCE handler 與應用函式為 strong definition；F407 OTG_FS／CDC、L476 USART2 handler 亦已連入。這只證明符號連結，不證明 MCU IRQ 實際行為。

## 電腦端測試

**五組 C tests 均已在 PC 的 WASI 環境實際執行並 PASS。** 原始輸出見 [host-tests.txt](host-tests.txt)，結構化狀態與來源雜湊見 [validation.json](validation.json)。測試直接編譯工程使用的可攜式 C 原始碼，沒有改寫一份 Python 協定當作替代：

| 測試 | 涵蓋行為 |
|---|---|
| protocol | 全 256 序號，全部 2048 standard ID、全部 256 個 DLC byte 值、HELLO／ACK 字元損壞、NULL 及失敗不覆寫輸出 |
| session | 768 次交換跨序號循環、stale／duplicate／wrong-type／late ACK、249/250 ms 邊界、uint32 tick wrap、缺號／重複與重新同步 |
| logging | 固定佇列 FIFO、busy 後重試、overflow、過長輸出、offline drop 與恢復 |
| USB transport mock | NULL class、未配置、DTR 未開、1000 次 busy、非同步 buffer 不覆寫、USBD_BUSY／FAIL、IRQ mask 恢復 |
| UART transport mock | 非同步專用 buffer、1000 次 busy、HAL_BUSY／ERROR、IRQ mask 恢復 |

原生 Windows protocol.exe 編譯後，在 CreateProcess 階段被拒絕，回傳 **WinError 4551：An Application Control policy has blocked this file**。因此原生 Windows 執行測試未通過，也不能稱為已完整執行五個原生測試。沒有關閉 Smart App Control、改防毒／Code Integrity、修改系統政策或解除檔案封鎖。TinyCC 官方下載另遇 HTTP 504，未安裝或執行成功。

本次測試命令使用 `--zig --wasm --wasi-sysroot`，連結 [WASI SDK 34.0 官方預建 libc](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34)，以避免首次 runtime 全部重建。保留 assertions、`-Wall -Wextra -Werror -pedantic` 與 UBSan trap。五組 C 編譯皆無警告；Node 印出 WASI ExperimentalWarning，已保留在原始紀錄中。另提供不帶 sysroot 的 `--zig --wasm`，把同一份 C tests 編成 wasm32-wasi 後在電腦端 Node.js 執行；runtime 不提供環境變數或 filesystem preopens。它驗證協定、狀態機及 mock transport；**不是 Cortex-M emulator，也不代表 Windows 原生 EXE 或 STM32 實體周邊測試**。[Node.js WASI 文件](https://nodejs.org/api/wasi.html)。

## 程式碼與 Git Diff 審查

- CAN clock：從生成的 clock tree 核對 F407 PCLK1=24 MHz、L476 PCLK1=16 MHz，再於執行時使用 HAL_RCC_GetPCLK1Freq 計算 prescaler 3／2，16 TQ、87.5% sample point。F407 USB PLLQ=48 MHz，以實板具備 8 MHz HSE 為前提。
- CAN filter：32-bit ID list 僅允許 0x101／0x102 standard DATA frame，軟體再檢查格式。硬體 ACK 與應用 ACK 分開計算；TX queued 與 TX complete 分開記錄。
- 非阻塞路徑：主迴圈有 RX 工作上限，log 不 busy-wait、不使用 HAL_UART_Transmit 的阻塞輸出；ISR 不呼叫 vsnprintf。
- USB buffer：copy-before-submit，class/config/DTR/TxState 檢查及提交與 USB IRQ 互斥，重置後重新檢查 pointer；busy 時不改寫正在傳送的獨立 buffer。
- UART buffer：只有 HAL state READY 時重用，USART2 IRQ 實際有 handler。
- 來源一致性：`tools/audit_sources.py` 已通過，兩份 Core app 副本與 protocol／firmware 原始碼一致，metadata、模式定義、Startup、Linker、AF9 與呼叫入口完整。
- Staged path 檢查：只允許 CAN_Board_to_Board，沒有 Debug／Release／Normal／Loopback／build、ELF／map／obj／exe／暫存檔。
- 自訂來源的 `git diff --cached --check` 乾淨。ST vendor／生成碼及參考 Startup 保留上游原有行尾空白；全樹 diff --check 對這些檔案仍有 whitespace 診斷，沒有把它們稱為 compiler warning 或隱藏成全面無警告。

## 尚未驗證及硬體前提

- 完全沒有執行實體 CAN Normal、MCU 內部 loopback、SWD、USB 枚舉、UART VCP、bus-off 恢復或長時間測試。
- F407 只知 MCU 型號；實際板型、排針、晶振、USB 連接器與 VBUS 電路未知，不能聲稱已完成板級相容性確認。
- F407 目前明確使用 HSE 8 MHz，若實板為 25 MHz 或其他頻率，須先調 `.ioc`、HSE_VALUE、PLL 並重建，再接硬體。
- L476 工程使用 USART2 ST-LINK VCP，不使用 MCU 原生 USB CDC；需確認實板 SB13／SB14 與其他 shield 設定。
- 收發器、3.3 V 邏輯相容性、兩端 120 Ω、接地、線材、500 kbps 波形及 USB 48 MHz 精度尚未量測。
- 8-bit 序號有跨整個循環的舊包歧義，未提供生產級認證或持久化 replay 防護。
- CubeMX 6.19 再產生驗證及原生 Windows 測試 EXE 的可執行狀態仍未驗證。

後續硬體接線及逐步驗收流程已列在 [繁體中文 README](../README.md)，本週不執行其中的燒錄及硬體步驟。Pin、VCP、時脈與物理層的官方參考網址亦收錄於 README。

## Git 交付

使用既有 `can-f407-l476-software`；起始 Commit `970ff155d320a595897d7a35c801ce1f43c8faa4`。推送前遠端 main 為 `e1841804570e1401c64588e9034ff7fe6f75e4a0`。最終 Commit 與遠端核對結果由本次完成訊息提供；本報告不內嵌自己的 Commit SHA，避免自我引用。

[遠端 CAN 分支](https://github.com/LYQjimmy/git-intro/tree/can-f407-l476-software/CAN_Board_to_Board)。只推送指定分支，不 force push、不修改 main、不 merge PR。
