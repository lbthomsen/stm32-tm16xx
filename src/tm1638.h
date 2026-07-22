/*
 * tm1638.h
 *
 * Created on: Jul 14, 2026
 * Author: lth
 */

#ifndef TM1638_H_
#define TM1638_H_

#include "main.h"

typedef enum {
    tm1638_ok = 0,
    tm1638_err = 1
} tm1638_result_t;

// Open-Drain Bit-Bang Handle
typedef struct {
    GPIO_TypeDef *clk_port;
    uint16_t      clk_pin;
    GPIO_TypeDef *dio_port;
    uint16_t      dio_pin;
    GPIO_TypeDef *cs_port;
    uint16_t      cs_pin;
} tm1638_handle_t;

/**
 * @brief Initialize the TM1638 module in Open-Drain GPIO mode.
 * @param tm1638 Pointer to the driver handle.
 * @param clk_port GPIO Port for CLK (e.g., GPIOB).
 * @param clk_pin GPIO Pin for CLK (e.g., GPIO_PIN_10).
 * @param dio_port GPIO Port for DIO (e.g., GPIOB).
 * @param dio_pin GPIO Pin for DIO (e.g., GPIO_PIN_15).
 * @param cs_port GPIO Port for STB/CS (e.g., GPIOB).
 * @param cs_pin GPIO Pin for STB/CS (e.g., GPIO_PIN_12).
 * @param brightness Display brightness level (0-7).
 */
tm1638_result_t tm1638_init(tm1638_handle_t *tm1638,
                            GPIO_TypeDef *clk_port, uint16_t clk_pin,
                            GPIO_TypeDef *dio_port, uint16_t dio_pin,
                            GPIO_TypeDef *cs_port, uint16_t cs_pin,
                            uint8_t brightness);

/**
 * @brief Clear all 7-segment display digits and LEDs.
 */
tm1638_result_t tm1638_clear(tm1638_handle_t *tm1638);

/**
 * @brief Display a number (0-9, dot, or minus) on a specific digit position.
 * @param pos Digit position (0 to 7).
 * @param val Index from tm1638_digit array (0-9, 10=dot, 11=-).
 */
tm1638_result_t tm1638_set_digit(tm1638_handle_t *tm1638, uint8_t pos, uint8_t val);

/**
 * @brief Turn an individual discrete LED on or off.
 * @param pos LED index (0 to 7).
 * @param status 1 for ON, 0 for OFF.
 */
tm1638_result_t tm1638_set_led(tm1638_handle_t *tm1638, uint8_t pos, uint8_t status);

/**
 * @brief Read button states from the key-scan matrix.
 * @param buttons_state Pointer to store the 8-bit button mask (S1-S8).
 */
tm1638_result_t tm1638_read_buttons(tm1638_handle_t *tm1638, uint8_t *buttons_state);

#endif /* TM1638_H_ */
