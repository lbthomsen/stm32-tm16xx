/*
 * tm1638.c
 *
 * Created on: Jul 14, 2026
 * Author: lth
 */

#include "main.h"
#include "tm1638.h"

const uint8_t tm1638_digit[] = {
        0x3F, // 0
        0x06, // 1
        0x5B, // 2
        0x4F, // 3
        0x66, // 4
        0x6D, // 5
        0x7D, // 6
        0x07, // 7
        0x7F, // 8
        0x6F, // 9
        0x80, // dot
        0x40  // -
};

// Microsecond-scale delay helper for Open-Drain rise/fall edges
static void tm1638_delay(void) {
    for (volatile uint32_t i = 0; i < 1000; i++) {
        __NOP();
    }
}

inline static void tm1638_cs_on(tm1638_handle_t *tm1638) {
    HAL_GPIO_WritePin(tm1638->cs_port, tm1638->cs_pin, GPIO_PIN_RESET);
    tm1638_delay();
}

inline static void tm1638_cs_off(tm1638_handle_t *tm1638) {
    HAL_GPIO_WritePin(tm1638->cs_port, tm1638->cs_pin, GPIO_PIN_SET);
    tm1638_delay();
}

// Write 1 byte LSB-First in Open-Drain
static void tm1638_write_byte(tm1638_handle_t *tm1638, uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(tm1638->clk_port, tm1638->clk_pin, GPIO_PIN_RESET);

        if (byte & (1 << i)) {
            // High = Release line to let 5V pull-up take over
            HAL_GPIO_WritePin(tm1638->dio_port, tm1638->dio_pin, GPIO_PIN_SET);
        } else {
            // Low = Actively pull to GND
            HAL_GPIO_WritePin(tm1638->dio_port, tm1638->dio_pin, GPIO_PIN_RESET);
        }
        tm1638_delay();

        HAL_GPIO_WritePin(tm1638->clk_port, tm1638->clk_pin, GPIO_PIN_SET);
        tm1638_delay();
    }
}

// Read 1 byte LSB-First in Open-Drain
static uint8_t tm1638_read_byte(tm1638_handle_t *tm1638) {
    uint8_t byte = 0;

    // Release open-drain DIO pin so TM1638 can drive the 5V line
    HAL_GPIO_WritePin(tm1638->dio_port, tm1638->dio_pin, GPIO_PIN_SET);
    tm1638_delay();

    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(tm1638->clk_port, tm1638->clk_pin, GPIO_PIN_RESET);
        tm1638_delay();

        if (HAL_GPIO_ReadPin(tm1638->dio_port, tm1638->dio_pin) == GPIO_PIN_SET) {
            byte |= (1 << i);
        }

        HAL_GPIO_WritePin(tm1638->clk_port, tm1638->clk_pin, GPIO_PIN_SET);
        tm1638_delay();
    }
    return byte;
}

tm1638_result_t tm1638_clear(tm1638_handle_t *tm1638) {
    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x40); // Auto-increment address mode
    tm1638_cs_off(tm1638);

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0xC0); // Memory start address
    for (uint8_t i = 0; i < 16; i++) {
        tm1638_write_byte(tm1638, 0x00);
    }
    tm1638_cs_off(tm1638);

    return tm1638_ok;
}

tm1638_result_t tm1638_init(tm1638_handle_t *tm1638,
                            GPIO_TypeDef *clk_port, uint16_t clk_pin,
                            GPIO_TypeDef *dio_port, uint16_t dio_pin,
                            GPIO_TypeDef *cs_port, uint16_t cs_pin,
                            uint8_t brightness) {
    tm1638->clk_port = clk_port;
    tm1638->clk_pin  = clk_pin;
    tm1638->dio_port = dio_port;
    tm1638->dio_pin  = dio_pin;
    tm1638->cs_port  = cs_port;
    tm1638->cs_pin   = cs_pin;

    // Set high state (Release lines to 5V pull-ups)
    HAL_GPIO_WritePin(tm1638->cs_port, tm1638->cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(tm1638->clk_port, tm1638->clk_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(tm1638->dio_port, tm1638->dio_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    // Force Display OFF first to cycle state
    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x80);
    tm1638_cs_off(tm1638);

    // Clear display registers
    tm1638_clear(tm1638);

    // Set brightness & turn Display ON
    if (brightness > 7) brightness = 7;
    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x88 | brightness);
    tm1638_cs_off(tm1638);

    return tm1638_ok;
}

tm1638_result_t tm1638_set_digit(tm1638_handle_t *tm1638, uint8_t pos, uint8_t val) {
    if (pos > 7 || val > 11) return tm1638_err;

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x44); // Fixed address mode
    tm1638_cs_off(tm1638);

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0xC0 | (pos * 2));
    tm1638_write_byte(tm1638, tm1638_digit[val]);
    tm1638_cs_off(tm1638);

    return tm1638_ok;
}

tm1638_result_t tm1638_set_led(tm1638_handle_t *tm1638, uint8_t pos, uint8_t status) {
    if (pos > 7) return tm1638_err;

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x44);
    tm1638_cs_off(tm1638);

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0xC0 | ((pos * 2) + 1));
    tm1638_write_byte(tm1638, status ? 0x01 : 0x00);
    tm1638_cs_off(tm1638);

    return tm1638_ok;
}

tm1638_result_t tm1638_read_buttons(tm1638_handle_t *tm1638, uint8_t *buttons_state) {
    if (buttons_state == NULL) return tm1638_err;

    tm1638_cs_on(tm1638);
    tm1638_write_byte(tm1638, 0x42); // Read key scan matrix

    uint8_t rx_data[4];
    for (uint8_t i = 0; i < 4; i++) {
        rx_data[i] = tm1638_read_byte(tm1638);
    }
    tm1638_cs_off(tm1638);

    uint8_t keys = 0;
    for (uint8_t i = 0; i < 4; i++) {
        keys |= ((rx_data[i] & 0x01) << i);
        keys |= ((rx_data[i] & 0x10) >> 1) << i;
    }

    *buttons_state = keys;
    return tm1638_ok;
}
