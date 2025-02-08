UART to WebSocket Integration (ESP32)
flow to run this code

step 1) Build the Project:
a) Run idf.py build to build the project.
b)For a new project, c)Use idf.py create-project project1. step 2) idf.py menuconfig (this should be in code file directory) a)GUI will open b)Navigate to EXAMPLE CONNECTION CONFIGURATION and set the WiFi SSID (WiFi name) and WiFi Password (Ensure you use the correct WiFi credentials, as they are case-sensitive). step 3)idf.py set-target esp32s3/esp32 step 4) idf.py flash step 5) idf.py monitor

a)In the output, you will receive an IPv4 address (e.g., XXX.XXX.XXX.XXX). b)Copy and paste this IP address into your browser’s URL bar and press Enter. c)In the same URL, append /ws at the end (e.g., XXX.XXX.XXX.XXX/ws) and press Enter. d)If the connection is successful, you should see confirmation. Copy XXX.XXX.XXX.XXX/ws and paste it into Postman WebSocket or a Simple WebSocket Client extension. step 6) Now you are ready to go
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
