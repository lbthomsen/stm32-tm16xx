/**
 ******************************************************************************
 * @file           : tm1638.c
 * @brief          : TM1638 library source
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STM32World <lth@stm32world.com>
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "tm1638.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

// --- Private Constant Definitions ---

/** @brief Command to set data write mode (Auto-Increment Address Mode). */
static const uint8_t CMD_DATA_SET_AUTO_INC = 0x40;

/** @brief Command to set data write mode (Fixed Address Mode). */
static const uint8_t CMD_DATA_SET_FIXED = 0x44;

/** @brief Command to read key scan data. */
static const uint8_t CMD_DATA_READ = 0x42;

/** @brief Command base to set display address (0xC0 to 0xCF). */
static const uint8_t CMD_ADDRESS_SET = 0xC0;

/** @brief Command base to control display power and brightness. */
static const uint8_t CMD_DISPLAY_CTRL = 0x80;
static const uint8_t DISPLAY_ON_MASK = 0x08;
static const uint8_t DISPLAY_BRIGHTNESS_MASK = 0x07;

// --- Microsecond Hardware Delay ---

/**
 * @brief Provides accurate microsecond delay using Cortex-M DWT Cycle Counter.
 * Essential for Open-Drain rise-time management with 10k pull-up resistors.
 * @param us Microseconds to delay.
 */
static void tm1638_delay_us(uint32_t us) {
    uint32_t startTicks = DWT->CYCCNT;
    // SystemCoreClock is 168000000 on F407.
    // (SystemCoreClock / 1000000U) gives clock cycles per microsecond (168 cycles)
    uint32_t ticks = us * (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - startTicks) < ticks) {
        // Wait until clock cycles pass
    }
}

// --- Low-Level GPIO Functions ---

static inline void tm1638_sdo_high(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_SET);
}
static inline void tm1638_sdo_low(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_RESET);
}
static inline void tm1638_stb_high(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_SET);
}
static inline void tm1638_stb_low(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_RESET);
}
static inline void tm1638_clk_high(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_SET);
}
static inline void tm1638_clk_low(tm1638_handle_t *tm) {
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_RESET);
}

// --- Private Function Prototypes ---

static void tm1638_start_transmission(tm1638_handle_t *tm);
static void tm1638_end_transmission(tm1638_handle_t *tm);
static void tm1638_send_data(tm1638_handle_t *tm, uint8_t data);
static void tm1638_send_command(tm1638_handle_t *tm, uint8_t cmd);
static uint8_t char_to_segment_code(char c);

// --- Protocol Implementation ---

/**
 * @brief Initiates a data transmission sequence by pulling STB low.
 * Includes a setup delay to meet TM1638 timing requirements.
 */
static void tm1638_start_transmission(tm1638_handle_t *tm) {
    tm1638_stb_low(tm);
    tm1638_delay_us(2); // Setup time for STB falling edge
}

/**
 * @brief Terminates a transmission sequence by pulling STB high.
 * Includes hold and pulse-width delays for Open-Drain signal settling.
 */
static void tm1638_end_transmission(tm1638_handle_t *tm) {
    tm1638_delay_us(2); // Hold time before driving STB high
    tm1638_stb_high(tm);
    tm1638_delay_us(2); // Minimum high pulse width
}

/**
 * @brief Sends a single command byte to the TM1638.
 */
static void tm1638_send_command(tm1638_handle_t *tm, uint8_t cmd) {
    tm1638_start_transmission(tm);
    tm1638_send_data(tm, cmd);
    tm1638_end_transmission(tm);
}

/**
 * @brief Sends a byte of data to the TM1638, LSB first.
 * Delays are included to compensate for RC rise times in Open-Drain mode.
 */
static void tm1638_send_data(tm1638_handle_t *tm, uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        tm1638_clk_low(tm);

        if (data & 0x01) {
            tm1638_sdo_high(tm);
        } else {
            tm1638_sdo_low(tm);
        }

        tm1638_delay_us(1); // Allow open-drain signal to charge pull-up

        tm1638_clk_high(tm);
        tm1638_delay_us(1); // Hold clock high for sampling edge

        data >>= 1;
    }
}

// --- Public API Implementation ---

/**
 * @brief Initializes the TM1638 module, hardware timers, clears display, and sets brightness.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness Initial brightness level (0-7).
 */
void tm1638_init(tm1638_handle_t *tm, GPIO_TypeDef *clk_port, uint16_t clk_pin, GPIO_TypeDef *dio_port, uint16_t dio_pin, GPIO_TypeDef *stb_port, uint16_t stb_pin, uint8_t brightness) {

    // Enable DWT Cycle Counter for microsecond delays
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    tm->clk_port = clk_port;
    tm->clk_pin = clk_pin;
    tm->dio_port = dio_port;
    tm->dio_pin = dio_pin;
    tm->stb_port = stb_port;
    tm->stb_pin = stb_pin;

    // Establish idle default HIGH state on all open-drain lines
    tm1638_stb_high(tm);
    tm1638_clk_high(tm);
    tm1638_sdo_high(tm);

    HAL_Delay(10);

    tm->brightness = brightness & DISPLAY_BRIGHTNESS_MASK;
    tm1638_display_clear(tm);
    tm1638_set_brightness(tm, tm->brightness);
}

/**
 * @brief Sets the display brightness.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness Brightness level, from 0 (dimmest) to 7 (brightest).
 */
void tm1638_set_brightness(tm1638_handle_t *tm, uint8_t brightness) {
    if (brightness > 7) {
        brightness = 7;
    }
    tm->brightness = brightness;
    uint8_t command = CMD_DISPLAY_CTRL | DISPLAY_ON_MASK | brightness;
    tm1638_send_command(tm, command);
}

/**
 * @brief Clears all 8 segment displays and turns off all status LEDs.
 * @param tm Pointer to the TM1638 handle.
 */
void tm1638_display_clear(tm1638_handle_t *tm) {
    // Set to Auto-Increment address mode
    tm1638_send_command(tm, CMD_DATA_SET_AUTO_INC);

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, CMD_ADDRESS_SET); // Start at base address 0xC0

    // Clear all 16 display registers (8 digit + 8 LED control registers)
    for (uint8_t i = 0; i < 16; i++) {
        tm1638_send_data(tm, 0x00);
    }
    tm1638_end_transmission(tm);
}

/**
 * @brief Sets raw 8-bit segment data for a single display digit.
 * @param tm Pointer to the TM1638 handle.
 * @param position Display position (1-8).
 * @param data Bitmask for segment segments (bit 7=DP, bits 6-0=G-A).
 */
void tm1638_set_segment(tm1638_handle_t *tm, uint8_t position, uint8_t data) {
    if (position < 1 || position > 8) {
        return;
    }

    // Must set Fixed Address Mode for individual register updating
    tm1638_send_command(tm, CMD_DATA_SET_FIXED);

    uint8_t address = CMD_ADDRESS_SET + (2 * (position - 1));

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, address);
    tm1638_send_data(tm, data);
    tm1638_end_transmission(tm);
}

/**
 * @brief Controls an individual LED on the board.
 * @param tm Pointer to the TM1638 handle.
 * @param position LED position (1-8).
 * @param on True to light LED, false to turn off.
 */
void tm1638_set_led(tm1638_handle_t *tm, uint8_t position, bool on) {
    if (position < 1 || position > 8) {
        return;
    }

    // Must set Fixed Address Mode for individual register updating
    tm1638_send_command(tm, CMD_DATA_SET_FIXED);

    uint8_t address = CMD_ADDRESS_SET + (2 * position) - 1;

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, address);
    tm1638_send_data(tm, on ? 0x01 : 0x00);
    tm1638_end_transmission(tm);
}

/**
 * @brief Displays a single character on a specific 7-segment digit.
 * @param tm Pointer to the TM1638 handle.
 * @param position Display position (1-8).
 * @param c Character to render.
 * @param dot True to enable decimal point LED on the digit.
 */
void tm1638_display_char(tm1638_handle_t *tm, uint8_t position, char c, bool dot) {
    if (position < 1 || position > 8) {
        return;
    }
    uint8_t hex_code = char_to_segment_code(c);
    if (dot) {
        hex_code |= 0x80;
    }
    tm1638_set_segment(tm, position, hex_code);
}

/**
 * @brief Displays a string right-aligned across 7-segment digits.
 * @param tm Pointer to the TM1638 handle.
 * @param str Null-terminated string. Parses dots seamlessly.
 */
void tm1638_display_txt(tm1638_handle_t *tm, const char *str) {
    char display_buf[9] = "        ";
    bool dots[8] = { false };
    int8_t len = strlen(str);
    int8_t str_idx = len - 1;
    int8_t buf_idx = 7;

    while (str_idx >= 0 && buf_idx >= 0) {
        if (str[str_idx] == '.') {
            if (buf_idx < 7) {
                dots[buf_idx + 1] = true;
            }
        } else {
            display_buf[buf_idx] = str[str_idx];
            buf_idx--;
        }
        str_idx--;
    }

    for (uint8_t i = 0; i < 8; i++) {
        tm1638_display_char(tm, i + 1, display_buf[i], dots[i]);
    }
}

/**
 * @brief Reads button inputs from the keypad matrix.
 * @param tm Pointer to the TM1638 handle.
 * @return Bitmask representing buttons pressed (Bit 0 = S1 ... Bit 7 = S8).
 */
uint8_t tm1638_scan_buttons(tm1638_handle_t *tm) {
    uint32_t raw_key_data = 0;
    uint8_t pressed_keys = 0;

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, CMD_DATA_READ);

    // CRITICAL: Release DIO line so the N-MOS turns off and
    // the external 10k pull-up can bring the line high for input reading!
    tm1638_sdo_high(tm);

    // Turnaround time delay (t_WAIT min 1us per datasheet)
    tm1638_delay_us(5);

    // Read 32 bits (4 bytes) of key matrix status
    for (int8_t i = 0; i < 32; i++) {
        tm1638_clk_low(tm);
        tm1638_delay_us(2); // Wait for TM1638 to output valid bit on falling edge

        if (HAL_GPIO_ReadPin(tm->dio_port, tm->dio_pin) == GPIO_PIN_SET) {
            raw_key_data |= (1UL << i);
        }

        tm1638_clk_high(tm);
        tm1638_delay_us(2);
    }

    tm1638_end_transmission(tm);

    //printf("RAW: 0x%08LX\n", raw_key_data);

    // Map TM1638 register bit locations to S1-S8 buttons
    if (raw_key_data & (1UL << 0))
        pressed_keys |= (1 << 0);  // S1
    if (raw_key_data & (1UL << 8))
        pressed_keys |= (1 << 1);  // S2
    if (raw_key_data & (1UL << 16))
        pressed_keys |= (1 << 2);  // S3
    if (raw_key_data & (1UL << 24))
        pressed_keys |= (1 << 3);  // S4
    if (raw_key_data & (1UL << 4))
        pressed_keys |= (1 << 4);  // S5
    if (raw_key_data & (1UL << 12))
        pressed_keys |= (1 << 5);  // S6
    if (raw_key_data & (1UL << 20))
        pressed_keys |= (1 << 6);  // S7
    if (raw_key_data & (1UL << 28))
        pressed_keys |= (1 << 7);  // S8

    return pressed_keys;
}

/**
 * @brief Blocking helper to wait for a key press and release.
 * @param tm Pointer to the TM1638 handle.
 * @return Pressed key index (1-8).
 */
uint8_t tm1638_read_key_blocking(tm1638_handle_t *tm) {
    uint8_t buttons = 0;
    while ((buttons = tm1638_scan_buttons(tm)) == 0) {
        HAL_Delay(20);
    }

    // Debounce wait on release
    while (tm1638_scan_buttons(tm) != 0) {
        HAL_Delay(20);
    }

    for (uint8_t i = 0; i < 8; i++) {
        if ((buttons >> i) & 1) {
            return i + 1;
        }
    }
    return 0;
}

// --- Font Map Helper ---

/**
 * @brief Converts ASCII character to 7-segment segment mapping bitmask.
 */
static uint8_t char_to_segment_code(char c) {
    switch (c) {
    // Digits
    case '0':
        return 0x3f;
    case '1':
        return 0x06;
    case '2':
        return 0x5b;
    case '3':
        return 0x4f;
    case '4':
        return 0x66;
    case '5':
        return 0x6d;
    case '6':
        return 0x7d;
    case '7':
        return 0x07;
    case '8':
        return 0x7f;
    case '9':
        return 0x6f;
        // Letters (uppercase)
    case 'A':
        return 0x77;
    case 'B':
        return 0x7f;
    case 'C':
        return 0x39;
    case 'D':
        return 0x3f;
    case 'E':
        return 0x79;
    case 'F':
        return 0x71;
    case 'G':
        return 0x7d;
    case 'H':
        return 0x76;
    case 'I':
        return 0x06;
    case 'J':
        return 0x0e;
    case 'L':
        return 0x38;
    case 'O':
        return 0x3f;
    case 'P':
        return 0x73;
    case 'S':
        return 0x6d;
    case 'U':
        return 0x3e;
        // Letters (lowercase)
    case 'a':
        return 0x5f;
    case 'b':
        return 0x7c;
    case 'c':
        return 0x58;
    case 'd':
        return 0x5e;
    case 'f':
        return 0x71;
    case 'g':
        return 0x6f;
    case 'h':
        return 0x74;
    case 'i':
        return 0x04;
    case 'n':
        return 0x54;
    case 'o':
        return 0x5c;
    case 'r':
        return 0x50;
    case 't':
        return 0x78;
    case 'u':
        return 0x1c;
    case 'y':
        return 0x6e;
        // Symbols
    case ' ':
        return 0x00;
    case '_':
        return 0x08;
    case '-':
        return 0x40;
    default:
        return 0x00;
    }
}
