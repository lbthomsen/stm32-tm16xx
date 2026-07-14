/*
 * tm1638.c
 *
 * Created on: Jul 14, 2026
 * Author: lth
 */

#include "main.h"
#include "tm1638.h"

// 168 MHz means ~6ns per clock cycle.
// 150 NOPs ≈ 900ns delay, establishing a bulletproof serial speed (< 500 kHz)
#define TM1638_DELAY() do { \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
} while(0)

// Explicitly configures all three TM1638 pins as Open-Drain outputs.
// This bypasses CubeMX pin assignment problems.
void TM1638_Hardware_Init(void) {
    // 1. Enable GPIO Clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 2. Configure STB Pin (PA8)
    GPIO_InitStruct.Pin = TM_STB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // Open-Drain to utilize board's pull-up
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TM_STB_GPIO_Port, &GPIO_InitStruct);

    // 3. Configure CLK Pin (PC8)
    GPIO_InitStruct.Pin = TM_CLK_Pin;
    HAL_GPIO_Init(TM_CLK_GPIO_Port, &GPIO_InitStruct);

    // 4. Configure DIO Pin (PC6)
    GPIO_InitStruct.Pin = TM_DIO_Pin;
    HAL_GPIO_Init(TM_DIO_GPIO_Port, &GPIO_InitStruct);

    // Set initial IDLE High baseline state
    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
}

void TM1638_WriteByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        // 1. Drop CLK Low
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
        TM1638_DELAY();

        // 2. Output the data bit while CLK is Low
        if (byte & 0x01) {
            HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);
        }
        byte >>= 1;
        TM1638_DELAY(); // Setup time

        // 3. Pull CLK High to commit the bit
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
        TM1638_DELAY(); // Hold time
    }
}

uint8_t TM1638_ReadByte(void) {
    uint8_t byte = 0;

    // Write high to releasing open-drain bus control, allowing TM1638 to drive line
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
    TM1638_DELAY();

    for (uint8_t i = 0; i < 8; i++) {
        // 1. Drop CLK Low
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
        TM1638_DELAY();

        // 2. Pull CLK High
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
        TM1638_DELAY();

        // 3. Read bit
        if (HAL_GPIO_ReadPin(TM_DIO_GPIO_Port, TM_DIO_Pin) == GPIO_PIN_SET) {
            byte |= (1 << i);
        }
        TM1638_DELAY();
    }
    return byte;
}

void TM1638_SendCommand(uint8_t command) {
    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_RESET);
    TM1638_DELAY();
    TM1638_WriteByte(command);
    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_SET);

    TM1638_DELAY(); // Critical processing transition margin
}

void TM1638_WriteData(uint8_t address, uint8_t data) {
    TM1638_SendCommand(0x44); // Command 2: Set fixed address mode

    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_RESET);
    TM1638_DELAY();
    TM1638_WriteByte(0xC0 | address); // Command 3: Write to designated address register
    TM1638_WriteByte(data);
    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_SET);

    TM1638_DELAY(); // Let data write settle internally
}

// 7-segment font map for numbers 0-9
const uint8_t SEGMENT_MAP[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void TM1638_Init(uint8_t brightness) {
    // 1. Force override hardware pins configuration
    TM1638_Hardware_Init();

    // Give physical pull-up resistors time to pull lines to 5V on startup
    HAL_Delay(50);

    if(brightness > 7) brightness = 7;
    TM1638_SendCommand(0x88 | brightness); // Command 4: Display ON + brightness

    // Clear display registers (16 write channels total)
    for(uint8_t i=0; i<16; i++) {
        TM1638_WriteData(i, 0x00);
    }
}

// Display a digit (0-9) on a specific position (0-7)
void TM1638_DisplayDigit(uint8_t position, uint8_t digit) {
    if (position > 7 || digit > 9) return;
    TM1638_WriteData(position * 2, SEGMENT_MAP[digit]);
}

// Control individual LEDs above the buttons (0 = off, 1 = on)
void TM1638_SetLED(uint8_t position, uint8_t status) {
    if (position > 7) return;
    TM1638_WriteData((position * 2) + 1, status ? 0x01 : 0x00);
}

// Read all 8 buttons. Returns a byte where each bit represents a button state.
uint8_t TM1638_ReadButtons(void) {
    uint8_t keys = 0;

    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_RESET);
    TM1638_DELAY();
    TM1638_WriteByte(0x42); // Command 2: Read key scan data

    TM1638_DELAY(); // Turnaround processing wait time

    uint8_t key_data[4];
    for (uint8_t i = 0; i < 4; i++) {
        key_data[i] = TM1638_ReadByte();
    }
    HAL_GPIO_WritePin(TM_STB_GPIO_Port, TM_STB_Pin, GPIO_PIN_SET);
    TM1638_DELAY();

    // Map 4-byte key packet back to single byte status (S1-S8)
    for (uint8_t i = 0; i < 4; i++) {
        keys |= ((key_data[i] & 0x01) << i);       // S1, S2, S3, S4
        keys |= ((key_data[i] & 0x10) >> 1) << i;  // S5, S6, S7, S8
    }

    return keys;
}
