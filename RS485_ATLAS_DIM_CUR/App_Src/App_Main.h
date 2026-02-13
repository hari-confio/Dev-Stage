/*
 * App_Main.h
 *
 *  Created on: 30-Jan-2023
 *      Author: Confio
 */
#ifndef APP_SRC_APP_MAIN_H_
#define APP_SRC_APP_MAIN_H_

    #include "app/framework/include/af.h"
    #include "stack/include/ember-types.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include "gpiointerrupt.h"
    #include "em_device.h"
    #include "em_cmu.h"
    #include "em_emu.h"
    #include "em_chip.h"
    #include "em_timer.h"
    #include "em_gpio.h"
    #include "bsp.h"

#define MAIN_FW_VER       '1'
#define MAJOR_BUG_FIX     '0'
#define MINOR_BUG_FIX     '2'


#define RX_HEADER_1                                          0x55
#define RX_HEADER_2                                          0xAA
#define RX_VERSION                                           0x02
#define RX_COMMAND                                           0x06
#define RX_RESET_CMD                                         0x03
#define RX_COMMAND_TYPE_ON_OFF                               0x01
#define RX_COMMAND_TYPE_LEVEL                                0x02
#define RX_CMD_ON                                            0x01
#define RX_CMD_OFF                                           0x00

#define SERIAL_TX_PORT                                       gpioPortB
#define SERIAL_TX_PIN                                        1

#define SERIAL_RX_PORT                                       gpioPortB
#define SERIAL_RX_PIN                                        0

#define RX_BUFF_LEN                                          64

#define USART2_TX_PORT                                       gpioPortD  //usart2
#define USART2_TX_PIN                                        4

#define USART2_RX_PORT                                       gpioPortD  //usart2
#define USART2_RX_PIN                                        3

#define READ_PORT               gpioPortC
#define READ_PIN                0

#define WRITE_PORT              gpioPortC
#define WRITE_PIN               5


typedef enum _SERIAL_RX_STATE_
{
  STATE_HEADER_1  = 1,
  STATE_HEADER_2,
  STATE_VERSION,
  STATE_SEQUENCE_1,
  STATE_SEQUENCE_2,
  STATE_COMMAND,
  STATE_LENGTH_1,
  STATE_LENGTH_2,
  STATE_DATA,
  STATE_CHECKSUM,
  STATE_ERROR,
  STATE_EXIT
} SERIAL_RX_STATE;

typedef enum _RX_DATA_TYPE_
{
  DATA_DPID = 0,
  DATA_TYPE,
  DATA_LEN_1,
  DATA_LEN_2,
  DATA_VAL
}RX_DATA_TYPE;

typedef enum _ENDPOINTS_
{
  Endpoint_1 = 1,
  Endpoint_2,
  Endpoint_3,
  Endpoint_4,
}ENDPOINTS;

typedef enum _BUTTON_NUMBER_
{
  Button_1 = 1,
  Button_2,
  Button_3,
  Rotatory_Button,
}BUTTON_NUMBER;


extern  uint8_t Broadcast_seq;
extern  uint16_t Boot_Count;
extern uint8_t BUTTON_STATUS;

extern EmberEventControl app_Button_1_StatusUpdateEventControl;
extern EmberEventControl app_Button_2_StatusUpdateEventControl;
extern EmberEventControl app_Button_3_StatusUpdateEventControl;
extern EmberEventControl app_Button_Rotatory_StatusUpdateEventControl;
extern EmberEventControl app_Button_Rotatory_UpdateEventControl;

#endif
