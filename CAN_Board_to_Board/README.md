# ATlab | STM32F407 ↔ STM32L476 CAN software draft

**Status (2026-11-24): Software source prepared for review. Neither hardware nor STM32CubeIDE firmware build has been verified.**
This work is on a separate branch and does **not** change the previous UART projects.

## Goal

- Target A: **STM32F407VET6** (sends HELLO, CAN ID `0x101`).
- Target B: **NUCLEO-L476RG / STM32L476RGT6** (receives HELLO and responds ACK, ID `0x102`).
- One CAN frame per second from F407. Both boards can operate in **internal loopback** mode for individual self-tests.
- Optional `CDC_Transmit_FS()` logging; this **requires a separately generated USB Device CDC stack and an actual usable MCU USB connector**. Without that option, the most recent log and counters are retained in RAM for debugger inspection.

## Files

```text
CAN_Board_to_Board/
├── protocol/
│   ├── atlab_can_protocol.h    # Portable classic-CAN message format
│   └── atlab_can_protocol.c
├── firmware/
│   ├── atlab_can_hal.h/.c      # HAL CAN1 init, filter, TX, polling RX
│   ├── atlab_demo.h/.c         # Per-board application behavior
│   └── atlab_debug_log.h/.c    # USB CDC option; RAM fallback
└── tests/
    ├── test_protocol.c
    └── run_tests.sh
```

## What you can check without hardware

The portable protocol can be tested on a computer with a C compiler:

```bash
bash tests/run_tests.sh
```

The test covers all 256 sequence numbers, HELLO/ACK decoding and invalid frames. **It does not test the STM32 HAL, USB CDC, electrical CAN bus or board-to-board communication.**

## Firmware integration in STM32CubeIDE (later)

The `firmware/` files are **source modules, not a complete generated CubeIDE project**.
Keep the previous `ATlab_UART_F407` and `ATlab_UART_L476` projects unchanged. In newly created projects for the correct MCU:

1. Generate a project with **STM32CubeMX / STM32CubeIDE**, selecting the corresponding MCU.
2. Include the STM32 HAL **CAN driver module**, including the relevant `stm32f4xx_hal_can.c` or `stm32l4xx_hal_can.c` and `HAL_CAN_MODULE_ENABLED` in HAL config. Ensure CAN1 peripheral support exists in the MCU firmware package.
3. Copy both `protocol/*.c` and `firmware/*.c` to `Core/Src`; copy both `protocol/*.h` and `firmware/*.h` to `Core/Inc`.
4. Configure `SystemClock_Config()` so that **PCLK1/APB1 = 16 MHz** (HSI 16 MHz is a convenient source on both targets). CAN uses prescaler 2, BS1 13 TQ, BS2 2 TQ: **500 kbit/s**, sample point 87.5%.
5. In `main.c` after `HAL_Init()` and `SystemClock_Config()`, call `ATLAB_Demo_Run();`. It is an infinite loop and initializes CAN1 itself; do not also call a generated `MX_CAN1_Init()` or create a competing global CAN1 handle.
6. This code configures **PB8 = CAN1_RX**, **PB9 = CAN1_TX**, alternate function AF9, in `ATLAB_CAN_BringUp()`. Those pins cannot simultaneously be used for another function. Debug SWD should stay enabled.
7. Default is `ATLAB_CAN_TEST_LOOPBACK=1` in `firmware/atlab_demo.c`. In this mode each MCU sees only **its own** transmitted frames. For later two-board work, build with `ATLAB_CAN_TEST_LOOPBACK=0` and provide compatible CAN transceivers, bus wiring and termination.
8. For actual **USB CDC** output, configure USB Device → CDC in CubeMX, generate USB middleware, initialize USB before `ATLAB_Demo_Run()`, and define `ATLAB_USE_USB_CDC` for compilation. USB clock/connector/DP-DM/VBUS wiring must be checked separately. The ST-LINK Virtual COM Port on the NUCLEO board is **UART**, not USB Device CDC.

### CAN electrical setup (not done this week)

In normal mode the two MCUs each require a compatible **CAN transceiver**.
Connect CAN_H ↔ CAN_H and CAN_L ↔ CAN_L, with a **120 Ω resistor across CAN_H and CAN_L at each physical bus end** (unless already fitted on the modules). Confirm voltage levels, ground reference, pinout and transceiver power before testing.

## Protocol

| Sender | CAN ID | DLC | Data bytes |
|---|---|---|---|
| F407 | `0x101` | 6 | `H E L L O seq` |
| L476 | `0x102` | 4 | `A C K seq` |

`seq` is an 8-bit counter that wraps after 255. The program polls CAN RX FIFO0 every 5 ms.
This is an **introductory demo**, not a production flight-software design. It does not implement message authentication, robust retry queues, CAN error-state recovery, protocol-version negotiation or deterministic scheduling.

## Verification checklist

- [ ] Run desktop C protocol test and record result.
- [ ] Generate/build F407 project with required CAN HAL source.
- [ ] Generate/build L476 project with required CAN HAL source.
- [ ] Check F407 loopback on hardware.
- [ ] Check L476 loopback on hardware.
- [ ] Configure and physically test USB CDC on each actual board (if available).
- [ ] Verify normal-mode board-to-board HELLO/ACK with transceivers and two 120 Ω terminations.

**Do not report CAN loopback, USB CDC or board-to-board success until those tests have actually run.**
