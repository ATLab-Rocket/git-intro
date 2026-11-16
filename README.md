# 自我介紹
- 你的名字:一句話自我介紹

---
李彥慶，太空系大一
# ATlab STM32 UART Board-to-Board Communication

## Project Overview

This project demonstrates bidirectional UART communication between two STM32 development boards.

- **STM32F407VET6:** Transmits `HELLO` and waits for `ACK`.
- **STM32L476RG:** Receives `HELLO` and responds with `ACK`.

## Hardware

- STM32F407VET6 development board
- NUCLEO-L476RG development board
- ST-LINK V2 programmer
- Jumper wires
- USB cables

## UART Configuration

| Parameter | Value |
|---|---|
| Peripheral | USART1 |
| Baud Rate | 9600 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Hardware Flow Control | None |

## Wiring

| STM32F407 | STM32L476 |
|---|---|
| PA9 (TX) | PA10 (RX / D2) |
| PA10 (RX) | PA9 (TX / D8) |
| GND | GND |

Both boards use their own USB power supplies. A common ground is required.

## Communication Protocol

1. STM32F407 sends `HELLO`.
2. STM32L476 validates the received message.
3. STM32L476 sends `ACK`.
4. STM32F407 verifies the reply.
5. The exchange repeats.

## Test Results

Observed using STM32CubeIDE Debug Expressions:

| Variable | Observed Value |
|---|---:|
| uart_ack_count | 17 |
| uart_error_count | 0 |
| uart_rx_error_count | 0 |

**Result:** Basic bidirectional UART communication was successfully demonstrated.

These values represent one observed test session, not a long-duration reliability test.

## Development Environment

- STM32CubeIDE 2.2.0
- STM32CubeMX
- STM32 HAL drivers
- STM32CubeProgrammer

## Project Files

- `ATlab_UART_F407/` — Transmitter and ACK verification firmware
- `ATlab_UART_L476/` — Receiver and ACK response firmware
- `docs/` — Wiring and test evidence

## Limitations

Long-duration reliability, packet loss, and recovery from communication failures have not yet been evaluated.
