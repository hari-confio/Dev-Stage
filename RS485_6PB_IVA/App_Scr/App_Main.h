/*
 * App_Main.h
 *
 *  Created on: 10-Jan-2023
 *      Author: Confio
 */

#ifndef APP_SCR_APP_MAIN_H_
#define APP_SCR_APP_MAIN_H_

#include "app/framework/include/af.h"
#include "stack/include/ember-types.h"
#include <stdio.h>
#include <stdlib.h>
#include "nvm3.h"

#define MAIN_FW_VER                '1'
#define MAJOR_BUG_FIX              '0'
#define MINOR_BUG_FIX              '0'

#define NVM3_KEY_B1SP   0x1001
#define NVM3_KEY_B2SP   0x1002
#define NVM3_KEY_B3SP   0x1003
#define NVM3_KEY_B4SP   0x1004
#define NVM3_KEY_B5SP   0x1005
#define NVM3_KEY_B6SP   0x1006
#define NVM3_KEY_B7SP   0x1007
#define NVM3_KEY_B8SP   0x1008
#define NVM3_KEY_B1DP   0x1009
#define NVM3_KEY_B2DP   0x100A
#define NVM3_KEY_B3DP   0x100B
#define NVM3_KEY_B4DP   0x100C
#define NVM3_KEY_B5DP   0x100D
#define NVM3_KEY_B6DP   0x100E
#define NVM3_KEY_B7DP   0x100F
#define NVM3_KEY_B8DP   0x1010
#define NVM3_KEY_B1LH   0x1011
#define NVM3_KEY_B2LH   0x1012
#define NVM3_KEY_B3LH   0x1013
#define NVM3_KEY_B4LH   0x1014
#define NVM3_KEY_B5LH   0x1015
#define NVM3_KEY_B6LH   0x1016
#define NVM3_KEY_B7LH   0x1017
#define NVM3_KEY_B8LH   0x1018

#define GPIO_BT1_PORT                       gpioPortA
#define GPIO_BT2_PORT                       gpioPortC
#define GPIO_BT3_PORT                       gpioPortC
#define GPIO_BT4_PORT                       gpioPortD
#define GPIO_BT5_PORT                       gpioPortD
#define GPIO_BT6_PORT                       gpioPortB

#define Button_1                            0
#define Button_2                            2
#define Button_3                            0
#define Button_4                            3
#define Button_5                            2
#define Button_6                            0

//#define LED_1                               4
//#define LED_2                               4
//#define LED_3                               0
//#define LED_4                               3
//#define LED_5                               0
//#define LED_6                               1
//
//#define GPIO_LED1_PORT                      gpioPortC
//#define GPIO_LED2_PORT                      gpioPortD
//#define GPIO_LED3_PORT                      gpioPortC
//#define GPIO_LED4_PORT                      gpioPortD
//#define GPIO_LED5_PORT                      gpioPortB
//#define GPIO_LED6_PORT                      gpioPortB

//    GPIO_PinModeSet(RE_PORT, RE_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(DE_PORT, DE_PIN, gpioModePushPull, 0);
//#define Button_7                            2
//#define Button_8                            4

#define KEYPAD_TYPE           0x66 //IVA 6PB

#define SERIAL_TX_PORT                                       gpioPortB   //USART1
#define SERIAL_TX_PIN                                        1

#define SERIAL_RX_PORT                                       gpioPortC  //USART2
#define SERIAL_RX_PIN                                        5
//
#define READ_PORT               gpioPortD
#define READ_PIN                4

#define WRITE_PORT              gpioPortC
#define WRITE_PIN               1

#define DIM_PORT                            gpioPortA
#define DIM_PIN                             4

#define PORT_SENSOR_INT                     MCP23017_GPIO_A//gpioPortB
#define PIN_SENSOR_INT                      0//0

#define Pressed                             true
#define Released                            false

#define ON                                  1
#define OFF                                 0

#define Led_Blink_Parameter                 0
#define Relay_Connection_Parameter          1

extern EmberEventControl Button_Isr_Poll_Control;
extern  uint8_t Broadcast_seq;
extern  uint16_t Boot_Count;
extern uint8_t Button_1_Count;


//extern EmberEventControl closeNetworkEventControl;
extern EmberEventControl Switch_Counter_Control;


typedef enum _ENDPOINTS_
{
  Endpoint_1 = 1,
  Endpoint_2,
  Endpoint_3,
  Endpoint_4,
  Endpoint_5,
  Endpoint_6,

}ENDPOINTS;




void Sensor_interrupt_CB(uint8_t id);

#endif /* APP_SCR_APP_MAIN_H_ */
