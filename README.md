UART to WebSocket Integration (ESP32)

Overview

This project integrates ESP32's UART communication with a WebSocket server. It allows bidirectional data transfer between a UART interface and a WebSocket client.

Features

Reads data from UART and sends it to WebSocket clients.

Receives data from WebSocket clients and transmits it via UART.

Uses FreeRTOS tasks for efficient handling of UART and WebSocket communication.

WebSocket server runs on ESP32, handling client connections and data exchange.

Hardware Requirements

ESP32 development board

UART device (e.g., another microcontroller or sensor)

USB cable for flashing and debugging

Software Requirements

ESP-IDF (ESP32 SDK)

CMake & Ninja build system

Python 3
