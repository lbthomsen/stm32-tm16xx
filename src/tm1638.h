/**
 ******************************************************************************
 * @file           : tm1638.h
 * @brief          : TM1638 library header
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

#ifndef TM1638_H_
#define TM1638_H_

#include <stdbool.h>
#include "stm32f4xx_hal.h"

/**
 * @brief Structure to hold the configuration for a TM1638 module.
 */
typedef struct {
    // GPIO Port and Pin for the CLK (Clock) line
    GPIO_TypeDef *clk_port;
    uint16_t clk_pin;

    // GPIO Port and Pin for the DIO (Data I/O) line
    GPIO_TypeDef *dio_port;
    uint16_t dio_pin;

    // GPIO Port and Pin for the STB (Strobe) line
    GPIO_TypeDef *stb_port;
    uint16_t stb_pin;

    // Current brightness level (0-7)
    uint8_t brightness;

} tm1638_handle_t;

// --- Public Function Prototypes ---

/**
 * @brief Initializes the TM1638 module. Must be called before any other function.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness Initial brightness level (0-7).
 */
void tm1638_init(tm1638_handle_t *tm, GPIO_TypeDef *clk_port, uint16_t clk_pin, GPIO_TypeDef *dio_port, uint16_t dio_pin, GPIO_TypeDef *stb_port, uint16_t stb_pin, uint8_t brightness);

/**
 * @brief Sets the brightness of the displays and LEDs.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness The brightness level, from 0 (dimmest) to 7 (brightest).
 */
void tm1638_set_brightness(tm1638_handle_t *tm, uint8_t brightness);

/**
 * @brief Clears all displays and turns off all LEDs.
 * @param tm Pointer to the TM1638 handle.
 */
void tm1638_display_clear(tm1638_handle_t *tm);

/**
 * @brief Displays a single character on a specified 7-segment display.
 * @param tm Pointer to the TM1638 handle.
 * @param position The display position (1-8, from left to right).
 * @param c The character to display (e.g., '0'-'9', 'A', 'b', etc.).
 * @param dot If true, the decimal point for that segment is turned on.
 */
void tm1638_display_char(tm1638_handle_t *tm, uint8_t position, char c, bool dot);

/**
 * @brief Displays a string on the 7-segment displays.
 *
 * The string is right-aligned. It automatically handles decimal points.
 * For example, "12.34" will be displayed as "  12.34".
 * If the string (excluding dots) is longer than 8 characters, it will be truncated.
 *
 * @param tm Pointer to the TM1638 handle.
 * @param str The null-terminated string to display.
 */
void tm1638_display_txt(tm1638_handle_t *tm, const char *str);


/**
 * @brief Controls a single LED.
 * @param tm Pointer to the TM1638 handle.
 * @param position The LED position (1-8, from left to right).
 * @param on True to turn the LED on, false to turn it off.
 */
void tm1638_set_led(tm1638_handle_t *tm, uint8_t position, bool on);

/**
 * @brief Sets the raw 8-bit segment data for a single display position.
 *
 * This allows for custom symbols or direct control over the segments.
 *
 * Bit mapping: 0b(DP)(G)(F)(E)(D)(C)(B)(A)
 *
 * For more info check: https://futuranet.it/futurashop/image/catalog/data/Download/TM1638_V1.3_EN.pdf (page 6)
 *
 * @param tm Pointer to the TM1638 handle.
 * @param position The display position (1-8).
 * @param data The 8-bit value representing the segments to light up.
 */
void tm1638_set_segment(tm1638_handle_t *tm, uint8_t position, uint8_t data);

/**
 * @brief Scans the keypad and returns a bitmask of pressed buttons.
 *
 * This is a non-blocking function.
 *
 * @param tm Pointer to the TM1638 handle.
 * @return An 8-bit mask of pressed keys. Bit 0 is S1, bit 1 is S2, ..., bit 7 is S8.
 *         A value of 0 means no keys are pressed.
 */
uint8_t tm1638_scan_buttons(tm1638_handle_t *tm);

/**
 * @brief Waits for a single key press and returns its number.
 *
 * This is a blocking function that includes debouncing.
 * It waits for a key to be pressed and then released.
 *
 * @param tm Pointer to the TM1638 handle.
 * @return The number of the key that was pressed (1-8).
 *         Returns 0 if multiple keys were pressed simultaneously.
 */
uint8_t tm1638_read_key_blocking(tm1638_handle_t *tm);

#endif /* TM1638_H_ */
