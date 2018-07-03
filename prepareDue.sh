#!/usr/bin/env bash
sed -i -- 's/void USART0_Handler(void)/void USART0_Handler(void ) __attribute__((weak));\nvoid USART0_Handler(void )/g' ~/.platformio/packages/framework-arduinosam/variants/arduino_due_x/variant.cpp
