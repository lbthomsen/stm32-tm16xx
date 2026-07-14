/*
 * tm1638.h
 *
 *  Created on: Jul 14, 2026
 *      Author: lth
 */

#ifndef TM1638_H_
#define TM1638_H_

typedef struct {

} tm1638_handler_t;

void TM1638_Init(uint8_t brightness);
void TM1638_DisplayDigit(uint8_t position, uint8_t digit);
void TM1638_SetLED(uint8_t position, uint8_t status);

#endif /* TM1638_H_ */
