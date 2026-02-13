/*
 * App_Main.h
 *
 *  Created on: Aug 24, 2022
 *      Author: Confio
 */

#ifndef APP_MAIN_H_
#define APP_MAIN_H_


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
    #include "em_iadc.h"
    #include "em_core.h"

    #define MAIN_FW_VER                '1'
    #define MAJOR_BUG_FIX              '0'
    #define MINOR_BUG_FIX              '3'

#define DEVICE_ID_1_ATTRIBUTE_ID                                 0x0016
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x0017
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x0018
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x0019
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x001A
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x001B
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x001C
#define DEVICE_ID_2_ATTRIBUTE_ID                                 0x001D
#define BUTTON_ID_2_ATTRIBUTE_ID                                 0x001E



//    #define GPIO_BT1_PORT             gpioPortD
//    #define GPIO_BT2_PORT             gpioPortC
//    #define GPIO_BT3_PORT             gpioPortA
//    #define GPIO_BT4_PORT             gpioPortC
//    #define GPIO_BT5_PORT             gpioPortC        //gpioPortD
//    #define GPIO_BT6_PORT             gpioPortD
//    #define GPIO_IDENTIFY_LED_PORT    gpioPortC        //gpioPortB
//
//    #define GPIO_BT1_PIN                      4
//    #define GPIO_BT2_PIN                      4
//    #define GPIO_BT3_PIN                      4
//    #define GPIO_BT4_PIN                      5
//    #define GPIO_BT5_PIN                      3
//    #define GPIO_BT6_PIN                      2
//    #define GPIO_IDENTIFY_LED_PIN             0

//    #define BUTTON_1                           1
//    #define BUTTON_2                           2
//    #define BUTTON_3                           3
//    #define BUTTON_4                           4
//    #define BUTTON_5                           5
//    #define BUTTON_6                           6

    #define GPIO_BT1_PORT             gpioPortD
    #define GPIO_BT2_PORT             gpioPortA
    #define GPIO_BT3_PORT             gpioPortC
    #define GPIO_BT4_PORT             gpioPortC
    #define GPIO_BT5_PORT             gpioPortC        //gpioPortD
    #define GPIO_BT6_PORT             gpioPortD
    #define GPIO_IDENTIFY_LED_PORT    gpioPortC        //gpioPortB

    #define GPIO_BT1_PIN                      4
    #define GPIO_BT2_PIN                      4
    #define GPIO_BT3_PIN                      3
    #define GPIO_BT4_PIN                      4
    #define GPIO_BT5_PIN                      5
    #define GPIO_BT6_PIN                      2
    #define GPIO_IDENTIFY_LED_PIN             0


    #define TURN_ON                           1
    #define TURN_OFF                          0

//extern bool dimVar;

    #define Max_Battery_Volt                  3000
    #define Min_Battery_Volt                  2400
    extern  uint8_t Broadcast_seq;
    extern uint16_t Boot_Count;
    extern bool Network_open;
    extern bool Battery_Per_Identifier;
    extern uint8_t Battery_Percent;
    extern uint8_t MACID[8];
    typedef enum _ENDPOINTS_
    {
      Endpoint_1 = 1,
      Endpoint_2,
      Endpoint_3,
      Endpoint_4,
      Endpoint_5,
      Endpoint_6,
    }ENDPOINTS;



    extern EmberEventControl commissioningLedEventControl;

    void AppGpioISRCallback(uint8_t pin, bool state);


#endif /* APP_MAIN_H_ */
