/*
 * MCP23017_I2C.h
 *
 *  Created on: 23-Nov-2023
 *      Author: Confio
 */

#ifndef SRC_MCP23017_2_I2C_H_
#define SRC_MCP23017_2_I2C_H_

#include <stdint.h>

//{0 1 0 0 A2 A1 A0 R/W}
#define MCP23017_ADDRESS_2    0x4E // A2 = A1 = A0 = 1 [0 1 0 0 1 1 1 0] = 0x4E



//  Registers                         //  DESCRIPTION                  DATASHEET
//Defined in MCP23017_I2C.h

#define APP_RELAY_1_PORT	  MCP23017_GPIO_B
#define APP_RELAY_1_PIN	    1//As per SCH

#define APP_RELAY_2_PORT	  MCP23017_GPIO_B
#define APP_RELAY_2_PIN	    5//As per SCH

#define APP_RELAY_3_PORT	  MCP23017_GPIO_B
#define APP_RELAY_3_PIN	    0//As per SCH

//Not Using these pins
#define APP_RELAY_4_PORT	  MCP23017_GPIO_B
#define APP_RELAY_4_PIN	    3

//Not Using these pins
#define APP_RELAY_5_PORT    MCP23017_GPIO_B
#define APP_RELAY_5_PIN     4

//Not Using these pins
#define APP_RELAY_6_PORT    MCP23017_GPIO_B
#define APP_RELAY_6_PIN     2//dummy replace for pin 5

//Not Using these pins
#define APP_RELAY_7_PORT    MCP23017_GPIO_A
#define APP_RELAY_7_PIN     7

//Not Using these pins
#define APP_RELAY_8_PORT    MCP23017_GPIO_A
#define APP_RELAY_8_PIN     6


void MCP23017_2_Init();
void MCP23017_2_GPIO_PinOutSet(uint8_t port, uint8_t pin, uint8_t value);
void MCP23017_2_GPIO_PinOutToggle(uint8_t port, uint8_t pin);


#endif /* SRC_MCP23017_I2C_H_ */
