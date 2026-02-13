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

extern EmberEventControl Button_Isr_Poll_Control;
extern bool Button_7_State;
extern bool Button_8_State;
extern bool btn7Hold;
extern bool btn8Hold;
extern bool btn7Press;
extern bool btn8Press;
extern bool btn7LongHoldScheduled;
extern bool btn8LongHoldScheduled;
extern int btn7Cnt;
extern int btn8Cnt;
extern int hold_btn;

#define KEYPAD_TYPE           0x06 //6PB

#define SERIAL_TX_PORT                                       gpioPortB   //USART1
#define SERIAL_TX_PIN                                        1

#define SERIAL_RX_PORT                                       gpioPortC  //USART2
#define SERIAL_RX_PIN                                        5

#define READ_PORT               gpioPortD
#define READ_PIN                4

  //#define WRITE_PORT              gpioPortC
  //#define WRITE_PIN               1
  #define WRITE_PORT              gpioPortD
  #define WRITE_PIN               4

extern EmberEventControl Button_7_8_press_Control;
extern int btn7PressCount;
extern bool btn7WaitingForDouble;


/*
 * Hardware V3
 *
 * PB1 -- PA0
 * PB2 -- PC2
 * PB3 -- PC1
 * PB4 -- PC0
 * PB5 -- PD4
 * PB6 -- PD3
 * PB7 -- PD2
 * PB8 -- PA4
 *
 * SCL -- PC4
 * SDA -- PC3
 *
 * INT -- PB0
 * DIM -- PB1
 *
 *
 */

#define MAIN_FW_VER                '1'
#define MAJOR_BUG_FIX              '0'
#define MINOR_BUG_FIX              '1'


#define GPIO_BT1_PORT                       gpioPortA
#define GPIO_BT2_PORT                       gpioPortC
#define GPIO_BT3_PORT                       gpioPortC
#define GPIO_BT4_PORT                       gpioPortC
#define GPIO_BT5_PORT                       gpioPortD
#define GPIO_BT6_PORT                       gpioPortD
#define GPIO_BT7_PORT                       MCP23017_GPIO_A
#define GPIO_BT8_PORT                       MCP23017_GPIO_A

#define Button_1                            0
#define Button_2                            2
#define Button_3                            1
#define Button_4                            0
#define Button_5                            3
#define Button_6                            2
#define Button_7                            7
#define Button_8                            6

#define DIM_PORT                            gpioPortA
#define DIM_PIN                             4

#define PORT_SENSOR_INT                     gpioPortB
#define PIN_SENSOR_INT                      0

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



extern EmberEventControl Switch_Counter_Control;
extern EmberEventControl Network_Steering_Event_Control;
extern EmberEventControl Button_Dimming_Control;

#define ENDPOINT_1               0x01
#define ENDPOINT_2               0x02
#define ENDPOINT_3               0x03
#define ENDPOINT_C4              0xC4

#define C4_BUTTON_PRESS_EVENT    129
#define C4_BUTTON_RELEASE_EVENT  130

#define START_UP_DELAY_SEC       3    //in Seconds
#define BUTTON_POLL_TIME_MS      100  //in Milli Second



void processReceivedData();
void sendFrame(uint8_t *data, size_t dataSize);
void set_btnSceneLed(uint8_t btnSceneLed);

#endif /* APP_SCR_APP_MAIN_H_ */
