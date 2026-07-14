/*
 * tm1638.c
 *
 *  Created on: Jul 14, 2026
 *      Author: lth
 */

#include "main.h"
#include "tm1638.h"

#define STB_GPIO_Port GPIOB
#define STB_Pin GPIO_PIN_0
#define CLK_GPIO_Port GPIOB
#define CLK_Pin GPIO_PIN_1
#define DIO_GPIO_Port GPIOB
#define DIO_Pin GPIO_PIN_2

// Set DIO pin mode dynamically
void TM1638_SetDIO_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DIO_GPIO_Port, &GPIO_InitStruct);
}

void TM1638_SetDIO_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // TM1638 requires pull-up when reading
    HAL_GPIO_Init(DIO_GPIO_Port, &GPIO_InitStruct);
}

void TM1638_WriteByte(uint8_t byte) {
    TM1638_SetDIO_Output();
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);

        if (byte & 0x01) {
            HAL_GPIO_WritePin(DIO_GPIO_Port, DIO_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(DIO_GPIO_Port, DIO_Pin, GPIO_PIN_RESET);
        }
        byte >>= 1;

        HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);
    }
}

uint8_t TM1638_ReadByte(void) {
    uint8_t byte = 0;
    TM1638_SetDIO_Input();
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
        // Small delay if your STM32 clock is very fast (>100MHz),
        // but standard HAL toggling is usually slow enough for TM1638.

        HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);

        if (HAL_GPIO_ReadPin(DIO_GPIO_Port, DIO_Pin) == GPIO_PIN_SET) {
            byte |= (1 << i);
        }
    }
    return byte;
}

void TM1638_SendCommand(uint8_t command) {
    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_RESET);
    TM1638_WriteByte(command);
    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_SET);
}

void TM1638_WriteData(uint8_t address, uint8_t data) {
    TM1638_SendCommand(0x44); // Command 2: Set fixed address mode

    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_RESET);
    TM1638_WriteByte(0xC0 | address); // Command 3: Set address
    TM1638_WriteByte(data);
    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_SET);
}

// 7-segment font map for numbers 0-9
const uint8_t SEGMENT_MAP[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void TM1638_Init(uint8_t brightness) {
    // brightness should be between 0 (lowest) and 7 (highest)
    if(brightness > 7) brightness = 7;
    TM1638_SendCommand(0x88 | brightness); // Command 4: Display control (ON + brightness)

    // Clear display
    for(uint8_t i=0; i<16; i++) {
        TM1638_WriteData(i, 0x00);
    }
}

// Display a digit (0-9) on a specific position (0-7)
void TM1638_DisplayDigit(uint8_t position, uint8_t digit) {
    if (position > 7 || digit > 9) return;
    // On standard TM1638 boards, digits are at even addresses (0x00, 0x02, 0x04...)
    TM1638_WriteData(position * 2, SEGMENT_MAP[digit]);
}

// Control the individual LEDs above the buttons (0 = off, 1 = on)
void TM1638_SetLED(uint8_t position, uint8_t status) {
    if (position > 7) return;
    // On standard boards, LEDs are at odd addresses (0x01, 0x03, 0x05...)
    TM1638_WriteData((position * 2) + 1, status ? 0x01 : 0x00);
}

// Read all 8 buttons. Returns a byte where each bit represents a button state.
uint8_t TM1638_ReadButtons(void) {
    uint8_t keys = 0;

    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_RESET);
    TM1638_WriteByte(0x42); // Command 2: Read key scan data

    // TM1638 returns 4 bytes of key data, but we only need a few bits for 8 keys
    uint8_t key_data[4];
    for (uint8_t i = 0; i < 4; i++) {
        key_data[i] = TM1638_ReadByte();
    }
    HAL_GPIO_WritePin(STB_GPIO_Port, STB_Pin, GPIO_PIN_SET);

    // Remap the 4 bytes into a single clean 8-bit byte for S1 to S8
    for (uint8_t i = 0; i < 4; i++) {
        keys |= ((key_data[i] & 0x01) << i);       // S1, S2, S3, S4
        keys |= ((key_data[i] & 0x10) >> 1) << i;  // S5, S6, S7, S8
    }

    return keys;
}


