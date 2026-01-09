# AutoPager

A BLE (Bluetooth Low Energy) keyboard device based on ESP32 that automatically sends page turn commands.

## Features

- Connects to devices via BLE as a keyboard
- Toggle auto-paging mode with a physical button
- Sends page turn commands at random intervals when in auto-paging mode
- LED status indicator for different system states

## Hardware

- ESP32 microcontroller
- Toggle button
- Status LED

## Dependencies

- NimBLE-Arduino
- ESP32 BLE Keyboard

## Usage

1. Connect the device via BLE
2. Press the button to start/stop auto-paging
3. The device will automatically send page turn commands
