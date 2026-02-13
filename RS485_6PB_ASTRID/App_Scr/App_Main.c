/*
 * App_Main.c
 *
 *  Created on: 10-Jan-2023
 *      Author: Confio
 */


/*
Button 1 (4 Times press) to include to the network
Button 1 (13 Times press) to exclude from the network
*/


#include "App_Scr/App_Main.h"
#include "App_Scr/App_StatusUpdate.h"
#include "MCP23017_I2C.h"
#include "MCP23017_2_I2C.h"
#include "App_LED.h"
#include "App_RELAY.h"
#include "../iic/driver_interface.h"
#include "gpiointerrupt.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "em_timer.h"
#include "em_usart.h"
#include "nvm3.h"
#include "nvm3_default.h"
#include "app/framework/plugin/network-creator-security/network-creator-security.h"
#include "app/framework/plugin/network-creator/network-creator.h"

uint16_t Boot_Count = 0;
uint8_t Broadcast_seq = 1;

EmberEventControl closeNetworkEventControl;
EmberEventControl Button_Isr_Poll_Control;
EmberEventControl appBroadcastSearchTypeEventControl;
EmberEventControl Button_Dimming_Control;
EmberEventControl Switch_Counter_Control;
EmberEventControl makeLedsToDefault_Control;

void processReceivedData();
void sendFrame(uint8_t *data, size_t dataSize);
void encryptAndSendData(uint32_t *data, int length);
void create_frame(int button, int action_type);

uint8_t active_button = 0;
uint8_t active_count = 0;
int btnSceneLed = 0;
static uint8_t start_up_delay = START_UP_DELAY_SEC*(BUTTON_POLL_TIME_MS/10);
static bool Start_Up_Done = 0;


//bool ledState11 = false, ledState21 = false, ledState31 = false, ledState41 = false, ledState51 = false, ledState61 = false, ledState71 = false, ledState81 = false;  // Toggle for active_count == 1
//bool ledState12 = false, ledState22 = false, ledState32 = false, ledState42 = false, ledState52 = false, ledState62 = false, ledState72 = false, ledState82 = false;  // Toggle for active_count == 2
//bool ledState1Long = false, ledState2Long = false, ledState3Long = false, ledState4Long = false, ledState5Long = false, ledState6Long = false, ledState7Long = false, ledState8Long = false;  // Toggle for long hold



#define RX_BUFF_LEN 64
volatile uint16_t txFrameLength;
uint8_t txSum = 0;
volatile uint16_t Rx_Buff[RX_BUFF_LEN] = {0};
static uint16_t Rx_Buff_Len = 0;
uint32_t Remote_Data[RX_BUFF_LEN] = {0};
#define RX_BUFF_SIZE 128

void closeNetworkEventHandler() {
  emberEventControlSetInactive(closeNetworkEventControl);
  emberAfPluginNetworkCreatorSecurityCloseNetwork();
  emberAfCorePrintln("---commissioningLedEventHandler--");
}

void Network_Steering_Event_Handler() {
  emberEventControlSetInactive(Network_Steering_Event_Control);

  // Check if the network is joined and open for joining
  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    EmberStatus status = emberAfPluginNetworkCreatorSecurityOpenNetwork();
    emberEventControlSetDelayMS(closeNetworkEventControl , 20000);
    if (status == EMBER_SUCCESS) {
      emberAfCorePrintln("Network is open for joining.");
    } else {
      emberAfCorePrintln("Failed to open network: 0x%X", status);
    }
    emberAfCorePrintln("->>Find and bind target start: 0x%X",
                       emberAfPluginFindAndBindTargetStart(1));
  } else {
    emberAfCorePrintln("Network is not formed yet.");
   // emberEventControlSetInactive(closeNetworkEventControl);
   // makeLedToDefault();
  }
 // emberEventControlSetInactive(closeNetworkEventControl);
  //makeLedToDefault();
  emberAfCorePrintln("Network steer Timeout");
}

void makeLedsToDefault_Handler()
{
  emberEventControlSetInactive(makeLedsToDefault_Control);
  led_set_to_white();
}

void USART2_RX_IRQHandler(void)
{
  //emberAfCorePrintln("USART2_RX_IRQHandler");

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);

    char data;

    // Start a timer (HW timer)
    TIMER_CounterSet(TIMER1, 1);
    TIMER_Enable(TIMER1, true);

    data = (char)USART_Rx(USART2);
    if (Rx_Buff_Len < RX_BUFF_SIZE) // Ensure no overflow
    {
        Rx_Buff[Rx_Buff_Len++] = data;
        //emberAfCorePrintln("%c ", data);
    }
    else
    {
        emberAfCorePrintln("Rx_Buff overflow");
    }
}

void TIMER1_IRQHandler(void)
{
  //emberAfCorePrintln("TIMER1_IRQHandler");
  uint32_t flags = TIMER_IntGet(TIMER1);
  TIMER_IntClear(TIMER1, flags);

  TIMER_Enable(TIMER1,false);
  processReceivedData();
  Rx_Buff_Len = 0;
}

void USART1_TX_IRQHandler(void) {

    uint32_t flags;
    flags = USART_IntGet(USART1);
    USART_IntClear(USART1, flags);

    if (flags & USART_IF_TXC)
    {
       // emberAfCorePrintln("Transmission complete");
    }

    GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);
    GPIO_PinOutClear(READ_PORT, READ_PIN);
}

void Init_Hw_Timer()
{
    emberAfCorePrintln("====Init_Hw_Timer");

    CMU_ClockEnable(cmuClock_TIMER1, true);

    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
    timerCCInit.mode = timerCCModeCompare;
    TIMER_InitCC(TIMER1, 0, &timerCCInit);

    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    timerInit.prescale = timerPrescale4;
    timerInit.enable = false;

    TIMER_Init(TIMER1, &timerInit);
    TIMER_TopSet(TIMER1, 0xFFFF);

    TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
    NVIC_EnableIRQ(TIMER1_IRQn);
}

void Serial_Init()
{
  //emberAfCorePrintln("====Serial_Init");

    // Enable clocks for USART1 (TX) and USART2 (RX)
    CMU_ClockEnable(cmuClock_USART1, true);
    CMU_ClockEnable(cmuClock_USART2, true);

    // Initialize USART1 for TX
    USART_InitAsync_TypeDef initTX = USART_INITASYNC_DEFAULT;
    initTX.baudrate = 9600;

    // Set pin modes for USART1 TX pin
    GPIO_PinModeSet(SERIAL_TX_PORT, SERIAL_TX_PIN, gpioModePushPull, 1);  // Tx
    GPIO_PinModeSet(READ_PORT, READ_PIN, gpioModePushPull, 0);  // Control GPIO
    GPIO_PinModeSet(WRITE_PORT, WRITE_PIN, gpioModePushPull, 0); // Control GPIO

    GPIO_PinOutClear(READ_PORT, READ_PIN);
    GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);

    // Route USART1 TX pin
    GPIO->USARTROUTE[1].TXROUTE = (SERIAL_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
                                | (SERIAL_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[1].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN;

    // Initialize USART1 in asynchronous mode
    USART_InitAsync(USART1, &initTX);

    // Initialize USART2 for RX
    USART_InitAsync_TypeDef initRX = USART_INITASYNC_DEFAULT;
    initRX.baudrate = 9600;

    // Set pin modes for USART2 RX pin
    GPIO_PinModeSet(SERIAL_RX_PORT, SERIAL_RX_PIN, gpioModeInput, 0);   // Rx
    GPIO->USARTROUTE[2].RXROUTE = (SERIAL_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                | (SERIAL_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[2].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN;

    // Initialize USART2 in asynchronous mode
    USART_InitAsync(USART2, &initRX);

    // Enable RX and TX interrupts for USART2 and USART1
    USART_IntEnable(USART2, USART_IEN_RXDATAV);
    NVIC_ClearPendingIRQ(USART2_RX_IRQn);
    NVIC_EnableIRQ(USART2_RX_IRQn);

    USART_IntEnable(USART1, USART_IEN_TXC);
    NVIC_ClearPendingIRQ(USART1_TX_IRQn);
    NVIC_EnableIRQ(USART1_TX_IRQn);

    // Initialize the hardware timer
    Init_Hw_Timer();
    //emberAfCorePrintln("====Serial_Init-2");

}



uint8_t MACID[8];

void processReceivedData()
{

  if(Rx_Buff[0] != 0)
  {
  emberAfCorePrintln("--Response--\n");
  for(int i=0; i<Rx_Buff_Len; i++)
    {
      emberAfCorePrint("%02X ", Rx_Buff[i]);
    }

  int mac_match_cnt = 0;
  // Convert Rx_Buff to a uint32_t array for decryption
  uint32_t decryptedFrame[14]; // Max 14 bytes based on your logic
  for (int i = 0; i < 14; i++)
  {
      decryptedFrame[i] = Rx_Buff[i];
  }

  // Decrypt the frame
  decryptFrame(decryptedFrame, 14);

  // Copy decrypted values back into Rx_Buff

  emberAfCorePrintln("-decrypted-Response--\n");

  for (int i = 0; i < 14; i++)
  {
      Rx_Buff[i] = (uint8_t)decryptedFrame[i];
      emberAfCorePrint("%02X ", Rx_Buff[i]);
  }
  emberAfCorePrint("\n");

 // 84 BA 20 FF FE 92 5E 5F 06 01 04 01 61 80
//1-8(MAC),9-5(RS485)

for(int i=0; i<8; i++)
  {
    if(Rx_Buff[i] == MACID[i])
      {
        mac_match_cnt++;
        if(mac_match_cnt == 7 && Rx_Buff[8] == 0x06) //6pb
          {
            // Proceed for further processing
            if(Rx_Buff[9] == 0x01) //BTN-1
              {
                if(Rx_Buff[10] == 0x04)//  ----Device Inclusion----
                  {
                    led_set_to_white();
                  }
                else if (Rx_Buff[10] == 0x0D) // ---- Device Exclusion ----
                {
                    led_set_to_white();

                    // Reset all LED states
//                    ledState11 = ledState21 = ledState31 = ledState41 = ledState51 = ledState61 = false;
//                    ledState12 = ledState22 = ledState32 = ledState42 = ledState52 = ledState62 = false;
//                    ledState1Long = ledState2Long = ledState3Long = ledState4Long = ledState5Long = ledState6Long = false;
                }
                 else if(Rx_Buff[10] == 0x01)
                 {
                    // ledState11 = !ledState11;  // Toggle LED state for Button == 1 & SinglePress
                     //set_led(1, Rx_Buff[11]);
//                     emberAfCorePrintln("--Response--BTN - %d\n", 1);
                     set_led(1, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState12 = !ledState12;  // Toggle LED state for Button == 1 & DoublePress
                     //set_led(1, Rx_Buff[11]);
                     set_led(1, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                   //  ledState1Long = !ledState1Long;  // Toggle LED state for for Button == 1 & long hold
                     //set_led(1, Rx_Buff[11]);
                     set_led(1, Rx_Buff[11]);
                 }
//                if(Rx_Buff[10] != 0x04 || Rx_Buff[10] != 0x0D){
//                set_led(1, Rx_Buff[11]);}
              }
            else if(Rx_Buff[9] == 0x02)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState21 = !ledState21;
                   // set_led(2, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState22 = !ledState22;
                   //  set_led(2, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                   //  ledState2Long = !ledState2Long;
                    // set_led(2, Rx_Buff[11]);
                 }
                 set_led(2, Rx_Buff[11]);
              }
            else if(Rx_Buff[9] == 0x03)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState31 = !ledState31;
                    // set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState32 = !ledState32;
                     //set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState3Long = !ledState3Long;
                     //set_led(3, Rx_Buff[11]);
                 }
                 set_led(3, Rx_Buff[11]);
              }
            else if(Rx_Buff[9] == 0x04)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState41 = !ledState41;
//                     set_led(4, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState42 = !ledState42;
                    // set_led(4, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState4Long = !ledState4Long;
                     //set_led(4, Rx_Buff[11]);
                 }
                 set_led(4, Rx_Buff[11]);
              }
            else if(Rx_Buff[9] == 0x05 || Rx_Buff[9] == 0x07) //BTN-5, 7
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState31 = !ledState31;
                     //set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState32 = !ledState32;
                     //set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState3Long = !ledState3Long;
                     //set_led(3, Rx_Buff[11]);
                 }
                 set_led(5, Rx_Buff[11]);
                 set_led(7, Rx_Buff[11]);
              }
            else if(Rx_Buff[9] == 0x06 || Rx_Buff[9] == 0x08) //BTN-6, 8
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState31 = !ledState31;
                     //set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState32 = !ledState32;
                     //set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState3Long = !ledState3Long;
                     //set_led(3, Rx_Buff[11]);
                 }
                 set_led(6, Rx_Buff[11]);
                 set_led(8, Rx_Buff[11]);
              }
//            else if(Rx_Buff[9] == 0x05)
//              {
//                 if(Rx_Buff[10] == 0x01)
//                 {
//                   //  ledState51 = !ledState51;
//                     set_led(5, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x02)
//                 {
//                    // ledState52 = !ledState52;
//                     set_led(5, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x03)
//                 {
//                    // ledState5Long = !ledState5Long;
//                     set_led(5, Rx_Buff[11]);
//                 }
//              }
//            else if(Rx_Buff[9] == 0x06)
//              {
//                 if(Rx_Buff[10] == 0x01)
//                 {
//                    // ledState61 = !ledState61;
//                     set_led(6, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x02)
//                 {
//                   // ledState62 = !ledState62;
//                     set_led(6, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x03)
//                 {
//                    // ledState6Long = !ledState6Long;
//                     set_led(6, Rx_Buff[11]);
//                 }
//              }
//            else if(Rx_Buff[9] == 0x07)
//              {
//                 if(Rx_Buff[10] == 0x01)
//                 {
//                    // ledState61 = !ledState61;
//                     set_led(7, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x02)
//                 {
//                   // ledState62 = !ledState62;
//                     set_led(7, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x03)
//                 {
//                    // ledState6Long = !ledState6Long;
//                     set_led(7, Rx_Buff[11]);
//                 }
//              }
//            else if(Rx_Buff[9] == 0x08)
//              {
//                 if(Rx_Buff[10] == 0x01)
//                 {
//                    // ledState61 = !ledState61;
//                     set_led(8, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x02)
//                 {
//                   // ledState62 = !ledState62;
//                     set_led(8, Rx_Buff[11]);
//                 }
//                 else if(Rx_Buff[10] == 0x03)
//                 {
//                    // ledState6Long = !ledState6Long;
//                     set_led(8, Rx_Buff[11]);
//                 }
//              }
      }

      }

    else
    {
        emberAfCorePrintln("------MAC ID Not Matched----\n");
    }
  }
  }


    memset(Rx_Buff, 0, sizeof(Rx_Buff)); // Clear buffer
    Rx_Buff_Len = 0; // Reset length

}

void led_set_to_white()
{

  set_proximity_level();
  MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);

  MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
}

void led_set_to_orange()
{

  set_proximity_level();
  MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 0);
  MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 0);

  MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 1);
  MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 1);
}

void init_relays_after_start_up_delay()
{
  bool onOff;
  if (emberAfReadServerAttribute (ENDPOINT_1,
                                  ZCL_ON_OFF_CLUSTER_ID,
                                  ZCL_ON_OFF_ATTRIBUTE_ID,
                                  (uint8_t*) &onOff, sizeof(onOff)) == EMBER_ZCL_STATUS_SUCCESS )
  {
      // emberAfCorePrintln("Endpoint = %d onOff = %d", ENDPOINT_1, onOff);
       set_relay_when_zigbee_event(1,onOff);
       set_led_when_zigbee_on_off_event(1,onOff);
       emberEventControlSetDelayMS(app_Relay1StatusUpdateEventControl,100);
  }

  onOff = 0;
  if (emberAfReadServerAttribute (ENDPOINT_2,
                                  ZCL_ON_OFF_CLUSTER_ID,
                                  ZCL_ON_OFF_ATTRIBUTE_ID,
                                  (uint8_t*) &onOff, sizeof(onOff)) == EMBER_ZCL_STATUS_SUCCESS )
  {
      // emberAfCorePrintln("Endpoint = %d onOff = %d", ENDPOINT_2, onOff);
       set_relay_when_zigbee_event(2,onOff);
       set_led_when_zigbee_on_off_event(2,onOff);
       emberEventControlSetDelayMS(app_Relay2StatusUpdateEventControl,100);
  }

  onOff = 0;
  if (emberAfReadServerAttribute (ENDPOINT_3,
                                  ZCL_ON_OFF_CLUSTER_ID,
                                  ZCL_ON_OFF_ATTRIBUTE_ID,
                                  (uint8_t*) &onOff, sizeof(onOff)) == EMBER_ZCL_STATUS_SUCCESS )
  {
      // emberAfCorePrintln("Endpoint = %d onOff = %d", ENDPOINT_3, onOff);
       set_relay_when_zigbee_event(3,onOff);
       set_led_when_zigbee_on_off_event(3,onOff);
       emberEventControlSetDelayMS(app_Relay3StatusUpdateEventControl,100);
  }
}

void init_c4_params_after_start_up_delay()
{
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_1_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_1,
      sizeof(C4_Led_1));
     // emberAfCorePrintln("C4_Led_1                  %d",C4_Led_1);

//      ID = C4_Led_1;
//   //   SCENE = 1;//Bt6_Switch1;
//      BUTTON = ZCL_C4_LED_1_ATTRIBUTE_ID;//cmd->buffer[3];
//      create_frame();

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_2_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_2,
      sizeof(C4_Led_2));
    //  emberAfCorePrintln("C4_Led_2                  %d",C4_Led_2);

//      ID = C4_Led_2;
//      create_frame();

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_3_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_3,
      sizeof(C4_Led_3));
     // emberAfCorePrintln("C4_Led_3                  %d",C4_Led_3);

//      ID = C4_Led_3;
//      create_frame();

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_4_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_4,
      sizeof(C4_Led_4));
     // emberAfCorePrintln("C4_Led_4                  %d",C4_Led_4);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_5_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_5,
      sizeof(C4_Led_5));
     // emberAfCorePrintln("C4_Led_5                  %d",C4_Led_5);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_6_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_6,
      sizeof(C4_Led_6));
     // emberAfCorePrintln("C4_Led_6                  %d",C4_Led_6);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_7_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_7,
      sizeof(C4_Led_7));
    //  emberAfCorePrintln("C4_Led_7                  %d",C4_Led_7);



      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_8_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_8,
      sizeof(C4_Led_8));
     // emberAfCorePrintln("C4_Led_8                  %d",C4_Led_8);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_1_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_1,
      sizeof(C4_Button_Relay_1));
     // emberAfCorePrintln("C4_Button_Relay_1        %d",C4_Button_Relay_1);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_2_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_2,
      sizeof(C4_Button_Relay_2));
     // emberAfCorePrintln("C4_Button_Relay_2        %d",C4_Button_Relay_2);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_3_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_3,
      sizeof(C4_Button_Relay_3));
     // emberAfCorePrintln("C4_Button_Relay_3        %d",C4_Button_Relay_3);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_4_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_4,
      sizeof(C4_Button_Relay_4));
     // emberAfCorePrintln("C4_Button_Relay_4        %d",C4_Button_Relay_4);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_5_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_5,
      sizeof(C4_Button_Relay_5));
     // emberAfCorePrintln("C4_Button_Relay_5        %d",C4_Button_Relay_5);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_6_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_6,
      sizeof(C4_Button_Relay_6));
      //emberAfCorePrintln("C4_Button_Relay_6        %d",C4_Button_Relay_6);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_7_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_7,
      sizeof(C4_Button_Relay_7));
      //emberAfCorePrintln("C4_Button_Relay_7        %d",C4_Button_Relay_7);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_BUTTON_8_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Button_Relay_8,
      sizeof(C4_Button_Relay_8));
     // emberAfCorePrintln("C4_Button_Relay_8        %d",C4_Button_Relay_8);


      uint8_t temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_1_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       1 %d",temp);
  //    set_led_when_zigbee_follow_connection_event(1, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_2_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       2 %d",temp);
    //  set_led_when_zigbee_follow_connection_event(2, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_3_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       3 %d",temp);
   //   set_led_when_zigbee_follow_connection_event(3, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_4_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
      //emberAfCorePrintln("follow_connection       4 %d",temp);
  //    set_led_when_zigbee_follow_connection_event(4, temp);

      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_5_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
   //   emberAfCorePrintln("follow_connection       5 %d",temp);
   //   set_led_when_zigbee_follow_connection_event(5, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_6_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       6 %d",temp);
    //  set_led_when_zigbee_follow_connection_event(6, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_7_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       7 %d",temp);
    //  set_led_when_zigbee_follow_connection_event(7, temp);


      temp = 0;
      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_8_STATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&temp,
      sizeof(temp));
     // emberAfCorePrintln("follow_connection       8 %d",temp);
   //   set_led_when_zigbee_follow_connection_event(8, temp);


      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_DIM_UP_RATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_Dim_Up_Rate,
      sizeof(C4_Led_Dim_Up_Rate));
     // emberAfCorePrintln("C4_Led_Dim_Up_Rate        %d",C4_Led_Dim_Up_Rate);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_DIM_DOWN_RATE_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_Dim_Down_Rate,
      sizeof(C4_Led_Dim_Down_Rate));
     // emberAfCorePrintln("C4_Led_Dim_Down_Rate      %d",C4_Led_Dim_Down_Rate);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_DIM_MIN_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_Dim_Min,
      sizeof(C4_Led_Dim_Min));
     // emberAfCorePrintln("C4_Led_Dim_Min            %d",C4_Led_Dim_Min);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_DIM_MAX_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_Dim_Max,
      sizeof(C4_Led_Dim_Max));
      //emberAfCorePrintln("C4_Led_Dim_Max            %d",C4_Led_Dim_Max);

      emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
      ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
      ZCL_C4_LED_ON_TIME_ATTRIBUTE_ID,
      EMBER_AF_MANUFACTURER_CODE,
      (uint8_t *)&C4_Led_On_Time,
      sizeof(C4_Led_On_Time));
      emberAfCorePrintln("C4_Led_On_Time1            %d",C4_Led_On_Time);

}

void init_after_start_up_delay()
{
  //Read C4 params
 // init_c4_params_after_start_up_delay();
  //Read OnOff server attribute and set IOs
 // init_relays_after_start_up_delay();
}

void factory_reset_attributes()
{
  C4_Led_1              = 0;
  C4_Led_2              = 0;
  C4_Led_3              = 0;
  C4_Led_4              = 0;
  C4_Led_5              = 0;
  C4_Led_6              = 0;
  C4_Led_7              = 0;
  C4_Led_8              = 0;
  C4_Button_Relay_1     = 0;
  C4_Button_Relay_2     = 0;
  C4_Button_Relay_3     = 0;
  C4_Button_Relay_4     = 0;
  C4_Button_Relay_5     = 0;
  C4_Button_Relay_6     = 0;
  C4_Button_Relay_7     = 0;
  C4_Button_Relay_8     = 0;
  C4_Led_Dim_Up_Rate    = 10;
  C4_Led_Dim_Down_Rate  = 10;
  C4_Led_Dim_Min        = 2;
  C4_Led_Dim_Max        = 100;
  C4_Led_On_Time        = 20;

//  emberAfCorePrintln("C4_Led_1                  %d",C4_Led_1);
//  emberAfCorePrintln("C4_Led_2                  %d",C4_Led_2);
//  emberAfCorePrintln("C4_Led_3                  %d",C4_Led_3);
//  emberAfCorePrintln("C4_Led_4                  %d",C4_Led_4);
//  emberAfCorePrintln("C4_Led_5                  %d",C4_Led_5);
//  emberAfCorePrintln("C4_Led_6                  %d",C4_Led_6);
//  emberAfCorePrintln("C4_Led_7                  %d",C4_Led_7);
//  emberAfCorePrintln("C4_Led_8                  %d",C4_Led_8);
//  emberAfCorePrintln("C4_Button_Relay_1         %d",C4_Button_Relay_1);
//  emberAfCorePrintln("C4_Button_Relay_2         %d",C4_Button_Relay_2);
//  emberAfCorePrintln("C4_Button_Relay_3         %d",C4_Button_Relay_3);
//  emberAfCorePrintln("C4_Button_Relay_4         %d",C4_Button_Relay_4);
//  emberAfCorePrintln("C4_Button_Relay_5         %d",C4_Button_Relay_5);
//  emberAfCorePrintln("C4_Button_Relay_6         %d",C4_Button_Relay_6);
//  emberAfCorePrintln("C4_Button_Relay_7         %d",C4_Button_Relay_7);
//  emberAfCorePrintln("C4_Button_Relay_8         %d",C4_Button_Relay_8);
  emberAfCorePrintln("C4_Led_Dim_Up_Rate        %d",C4_Led_Dim_Up_Rate);
  emberAfCorePrintln("C4_Led_Dim_Down_Rate      %d",C4_Led_Dim_Down_Rate);
  emberAfCorePrintln("C4_Led_Dim_Min            %d",C4_Led_Dim_Min);
  emberAfCorePrintln("C4_Led_Dim_Max            %d",C4_Led_Dim_Max);
  emberAfCorePrintln("C4_Led_On_Time3            %d",C4_Led_On_Time);

//  set_led(1, 0);
//  set_led(2, 0);
//  set_led(3, 0);
//  set_led(4, 0);
//  set_led(5, 0);
//  set_led(6, 0);
//  set_led(7, 0);
//  set_led(8, 0);
//  set_relay(1, 0);
//  set_relay(2, 0);
//  set_relay(3, 0);
//  set_relay(4, 0);
//  set_relay(5, 0);
//  set_relay(6, 0);
//  set_relay(7, 0);
//  set_relay(8, 0);

  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_1_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_1,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_2_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_2,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_3_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_3,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_4_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_4,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_5_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_5,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_6_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_6,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_7_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_7,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_8_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_8,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);


  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_1_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_1,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_2_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_2,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_3_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_3,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_4_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_4,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_5_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_5,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_6_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_6,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_7_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_7,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_BUTTON_8_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Button_Relay_8,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  uint8_t temp = 0;
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_1_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_2_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_3_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_4_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_5_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_6_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_7_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_8_STATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&temp,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);

  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_DIM_UP_RATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_Dim_Up_Rate,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_DIM_DOWN_RATE_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_Dim_Down_Rate,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_DIM_MIN_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_Dim_Min,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_DIM_MAX_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_Dim_Max,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_C4_LED_ON_TIME_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t *)&C4_Led_On_Time,
                                                  ZCL_INT8U_ATTRIBUTE_TYPE);
}

uint32_t btn1Buff[14], btn2Buff[14], btn3Buff[14], btn4Buff[14], btn5Buff[14], btn6Buff[14], btn7Buff[14], btn8Buff[14];
int led1State = 0, led2State = 0, led3State = 0, led4State = 0, led5State = 0, led6State = 0;

void Switch_Counter_Handler()
{
  emberEventControlSetInactive(Switch_Counter_Control);
  emberAfCorePrintln("active_button  %d %d", active_button, active_count);

  set_proximity_level();
  // Conversion to 6PB ASTRID
  if(active_button == 5 || active_button == 7)
    {
      active_button = 5;

    }
  if(active_button == 6 || active_button == 8)
    {
      active_button = 6;

    }

  if(active_button == 1 && active_count == 4)
      {
        emberAfCorePrintln("----Device Inclusion----");
        led_set_to_orange();
        btn1Buff[11] = 1;
        create_frame(1, active_count);
      }
    if (active_button == 1 && active_count>= 13)
      {
        emberAfCorePrintln("----Device Exclusion----");
        led_set_to_orange();
        btn1Buff[11] = 1;
        create_frame(1, active_count);
        emberEventControlSetDelayMS(makeLedsToDefault_Control , 2000);
      }
    if (active_button == 2 && active_count == 4)
    {
        led_set_to_orange();
      if (emberAfNetworkState() != EMBER_JOINED_NETWORK) {
        emberAfCorePrintln("Starting network formation...");
        emberAfPluginNetworkCreatorStart(1);
      } else {
        emberAfCorePrintln("Network already formed, opening network...");
        emberEventControlSetActive(Network_Steering_Event_Control);
      }
     // emberEventControlSetActive(closeNetworkEventControl);
      emberEventControlSetDelayMS(makeLedsToDefault_Control , 2000);
      emberEventControlSetDelayMS(Network_Steering_Event_Control, 4000);
      emberAfCorePrintln("Network Steering Started");
  //    emberEventControlSetActive(closeNetworkEventControl);

    }
    else if (active_button == 2 && active_count>= 13)
      {
       // if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
       // {
        EmberStatus status = emberLeaveNetwork();
        emberClearBindingTable();
        emberAfCorePrintln("Network Leave: 0x%X",status);
        led_set_to_orange();
        emberEventControlSetDelayMS(makeLedsToDefault_Control , 1000);
        emberEventControlSetDelayMS(Network_Steering_Event_Control , 2000);
       // }
      }
    else if (active_button == 1 || active_button == 50)
    {
        if (active_button == 1 && active_count == 1)
        {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

           // ledState11 = !ledState11;  // Toggle LED state for count == 1
           // set_led(1, ledState11);
            btn1Buff[11] = 1;//ledState11;
            create_frame(1, active_count);
        }
        else if (active_count == 2)
        {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

          //  ledState12 = !ledState12;  // Toggle LED state for count == 2
         //   set_led(1, ledState12);
            btn1Buff[11] = 1;//ledState12;
            create_frame(1, active_count);
        }
        else if (active_button == 50) // 50 indicates button long hold action
        {
            emberAfCorePrintln("---> Button %d Long Held\n", 1);

           // ledState1Long = !ledState1Long;  // Toggle LED state for long hold
           // set_led(1, ledState1Long);
            btn1Buff[11] = 1;//ledState1Long;
            create_frame(1, 3);
        }

//        led1State = !led1State;
//        set_led(1, led1State);
    }

    else if(active_button == 2 || active_button == 60)
      {
        if(active_button == 2 && active_count == 1)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
            //ledState21 = !ledState21;
            //set_led(2, ledState21);
            btn2Buff[11] = 1;//ledState21;
            create_frame(2, active_count);
          }
        else if(active_count == 2)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
          //  ledState22 = !ledState22;
          //  set_led(2, ledState22);
            btn2Buff[11] = 1;//ledState22;
            create_frame(2, active_count);
          }
        else if(active_button == 60)
          {
            emberAfCorePrintln("---> Button %d Long Held\n", 2);
           // ledState2Long = !ledState2Long;
           // set_led(2, ledState2Long);
            btn2Buff[11] = 1;//ledState2Long;
            create_frame(2, 3);
          }
       // led2State = !led2State;
       // set_led(2, led2State);
      }

    else if(active_button == 3 || active_button == 70)
      {
        if(active_button == 3 && active_count == 1)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState31 = !ledState31;
           // set_led(3, ledState31);
            btn3Buff[11] = 1;//ledState31;
            create_frame(3, active_count);
          }
        else if(active_count == 2)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState32 = !ledState32;
           // set_led(3, ledState32);
            btn3Buff[11] = 1;//ledState32;
            create_frame(3, active_count);
          }
        else if(active_button == 70)
          {
            emberAfCorePrintln("---> Button %d Long Held\n", 3);
            //ledState3Long = !ledState3Long;
           // set_led(3, ledState3Long);
            btn3Buff[11] = 1;//ledState3Long;
            create_frame(3, 3);
          }
       // led3State = !led3State;
      //  set_led(3, led3State);
      }
    else if(active_button == 4 || active_button == 80)
      {
        if(active_button == 4 && active_count == 1)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
          //  ledState41 = !ledState41;
          //  set_led(4, ledState41);
            btn4Buff[11] = 1;//ledState41;
            create_frame(4, active_count);
          }
        else if(active_count == 2)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState42 = !ledState42;
           // set_led(4, ledState42);
            btn4Buff[11] = 1;//ledState42;
            create_frame(4, active_count);
          }
        else if(active_button == 80)
          {
            emberAfCorePrintln("---> Button %d Long Held\n", 4);
           // ledState4Long = !ledState4Long;
           // set_led(4, ledState4Long);
            btn4Buff[11] = 1;//ledState4Long;
            create_frame(4, 3);
          }
      //  led4State = !led4State;
      //  set_led(4, led4State);
      }
    else if(active_button == 5 || active_button == 90 || active_button == 110)
      {
        if(active_button == 5 && active_count == 1)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState51 = !ledState51;
           // set_led(5, ledState51);
            btn5Buff[11] = 1;//ledState51;
            create_frame(5, active_count);
          }
        else if(active_count == 2)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState52 = !ledState52;
           // set_led(5, ledState52);
            btn5Buff[11] = 1;//ledState52;
            create_frame(5, active_count);
          }
        else if(active_button == 90 || active_button == 110)
          {
            emberAfCorePrintln("---> Button %d Long Held\n", 5);
           // ledState5Long = !ledState5Long;
          //  set_led(5, ledState5Long);
            btn5Buff[11] = 1;// ledState5Long;
            create_frame(5, 3);
          }
     //   led5State = !led5State;
     //   set_led(5, led5State);
     //   set_led(7, led5State);
      }
    else if(active_button == 6 || active_button == 100 || active_button == 120)
      {
        if(active_button == 6 && active_count == 1)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
          //  ledState61 = !ledState61;
           // set_led(6, ledState61);
            btn6Buff[11] = 1;//ledState61;
            create_frame(6, active_count);
          }
        else if(active_count == 2)
          {
            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
           // ledState62 = !ledState62;
           // set_led(6, ledState62);
            btn6Buff[11] = 1;//ledState62;
            create_frame(6, active_count);
          }
        else if(active_button == 100 || active_button == 120)
          {
            emberAfCorePrintln("---> Button %d Long Held\n", 6);
           // ledState6Long = !ledState6Long;
           // set_led(6, ledState6Long);
            btn6Buff[11] = 1;//ledState6Long;
            create_frame(6, 3);
          }
      //  led6State = !led6State;
      //  set_led(6, led6State);
      //  set_led(8, led6State);
      }
//    else if(active_button == 7 || active_button == 110)
//      {
//        if(active_button == 7 && active_count == 1)
//          {
//            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
//           // ledState71 = !ledState71;
//           // set_led(7, ledState71);
//            btn7Buff[11] = 1;//ledState71;
//            create_frame(7, active_count);
//          }
//        else if(active_count == 2)
//          {
//            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
//          //  ledState72 = !ledState72;
//           // set_led(7, ledState72);
//            btn7Buff[11] = 1;//ledState72;
//            create_frame(7, active_count);
//          }
//        else if(active_button == 110)
//          {
//            emberAfCorePrintln("---> Button %d Long Held\n", 7);
//           // ledState7Long = !ledState7Long;
//           // set_led(7, ledState7Long);
//            btn7Buff[11] = 1;//ledState7Long;
//            create_frame(7, 3);
//          }
//      }
//    else if(active_button == 8 || active_button == 120)
//      {
//        if(active_button == 8 && active_count == 1)
//          {
//            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
//           // ledState81 = !ledState81;
//           // set_led(8, ledState81);
//            btn8Buff[11] = 1;//ledState81;
//            create_frame(8, active_count);
//          }
//        else if(active_count == 2)
//          {
//            emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
//           // ledState82 = !ledState82;
//           // set_led(8, ledState82);
//            btn8Buff[11] = 1;//ledState82;
//            create_frame(8, active_count);
//          }
//        else if(active_button == 120)
//          {
//            emberAfCorePrintln("---> Button %d Long Held\n", 8);
//           // ledState8Long = !ledState8Long;
//            //set_led(8, ledState8Long);
//            btn8Buff[11] = 1;//ledState8Long;
//            create_frame(8, 3);
//          }
//      }
  active_button = 0;
  active_count = 0;
}


void start_timer_for_button_presses(uint8_t button)
{
  static uint8_t prv_button = 0;
  active_button = button;
  active_count++;
  if(prv_button != active_button)
  {
    prv_button = active_button;
    active_count=1;
  }
  emberEventControlSetDelayMS(Switch_Counter_Control, 500);
}

// Function to calculate Modbus CRC-16
uint16_t calculate_modbus_crc(uint32_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint8_t)data[i]; // Cast to uint8_t to process byte-wise
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void create_frame(int button, int action_type)
{
  uint32_t *activeBuff = NULL;

  if (button == 1)
   {
      activeBuff = btn1Buff;

   }
  else if (button == 2)
   {
      activeBuff = btn2Buff;
   }
  else if (button == 3)
   {
      activeBuff = btn3Buff;
   }
  else if (button == 4)
   {
      activeBuff = btn4Buff;
   }
  else if (button == 5)
   {
      activeBuff = btn5Buff;
   }
  else if (button == 6)
   {
      activeBuff = btn6Buff;
   }
  else if (button == 7)
   {
      activeBuff = btn7Buff;
   }
  else if (button == 8)
   {
      activeBuff = btn8Buff;
   }

  activeBuff[8] = KEYPAD_TYPE; //2pb
  activeBuff[9] = button;
 // emberAfCorePrint("activeBuff[9]-%X", activeBuff[9]);
  activeBuff[10] = action_type;
 // activeBuff[10] = btn_status;
  uint16_t crc = calculate_modbus_crc(activeBuff, 12);

  activeBuff[12] = crc & 0xFF;
  activeBuff[13] = (crc >> 8) & 0xFF;

  emberAfCorePrintln("-----BTN-%d-action-%d--> FRAME :\n", button, action_type);
  for (int i = 0; i < 14; i++) {
      emberAfCorePrint("%X ", activeBuff[i]);
  }
  emberAfCorePrintln("\n");

  uint32_t encryptedFrame[14];
  encryptFrameandSend(activeBuff, encryptedFrame);
}

static uint8_t add_sub_transform(uint8_t byte, int8_t value) {
    return (uint8_t)(byte + value);
}

void decryptFrame(uint32_t *frame, size_t len) {
    if (len < 14) return;

    // Revert Byte 0 to 7
    int8_t transforms[8] = {-9, 4, -5, 2, -1, 7, -9, 2};
    for (int i = 0; i < 8; i++) {
        frame[i] = add_sub_transform(frame[i], transforms[i]);
    }
    // Decode Byte 8
    uint8_t val8 = frame[8];
    if (val8 >= 1  && val8 <= 10) frame[8] = 0x02;
    else if (val8 >= 11 && val8 <= 20) frame[8] = 0x04;
    else if (val8 >= 21 && val8 <= 30) frame[8] = 0x06;
    else if (val8 >= 31 && val8 <= 40) frame[8] = 0x08;
    else if (val8 >= 41 && val8 <= 50) frame[8] = 0x66;
    else if (val8 >= 51 && val8 <= 60) frame[8] = 0x03;
    else if (val8 >= 61 && val8 <= 70) frame[8] = 0x55;
    // Decode Byte 9
    uint8_t val9 = frame[9];
    if      (val9 >= 1  && val9 <= 10) frame[9] = 0x01;
    else if (val9 >= 11 && val9 <= 20) frame[9] = 0x02;
    else if (val9 >= 21 && val9 <= 30) frame[9] = 0x03;
    else if (val9 >= 31 && val9 <= 40) frame[9] = 0x04;
    else if (val9 >= 41 && val9 <= 50) frame[9] = 0x05;
    else if (val9 >= 51 && val9 <= 60) frame[9] = 0x06;
    else if (val9 >= 61 && val9 <= 70) frame[9] = 0x07;
    else if (val9 >= 71 && val9 <= 80) frame[9] = 0x08;

    // Decode Byte 10
    uint8_t val10 = frame[10];
    if      (val10 >= 1  && val10 <= 20) frame[10] = 0x01;
    else if (val10 >= 21 && val10 <= 40) frame[10] = 0x02;
    else if (val10 >= 41 && val10 <= 60) frame[10] = 0x03;
    else if (val10 >= 61 && val10 <= 80) frame[10] = 0x04;
    else if (val10 >= 81 && val10 <= 99) frame[10] = 0x0D;

    // Decode Byte 11
    uint8_t val11 = frame[11];
    if      (val11 >= 1  && val11 <= 50) frame[11] = 0x00;
    else if (val11 >= 51 && val11 <= 99) frame[11] = 0x01;

    // Revert Byte 12 and 13
    frame[12] = add_sub_transform(frame[12], -6);
    frame[13] = add_sub_transform(frame[13], 3);
}


uint16_t random_n_to_m(uint16_t n, uint16_t m) {
    return (n + (halCommonGetRandom() % (m - n + 1)));
}

void encryptFrameandSend(uint32_t *originalFrame, uint32_t *encryptedFrame) {
    encryptedFrame[0] = originalFrame[0] + 9;
    encryptedFrame[1] = originalFrame[1] - 4;
    encryptedFrame[2] = originalFrame[2] + 5;
    encryptedFrame[3] = originalFrame[3] - 2;
    encryptedFrame[4] = originalFrame[4] + 1;
    encryptedFrame[5] = originalFrame[5] - 7;
    encryptedFrame[6] = originalFrame[6] + 9;
    encryptedFrame[7] = originalFrame[7] - 2;

    uint8_t type0 = originalFrame[8];
    if (type0 == 0x02) encryptedFrame[8] = random_n_to_m(1, 10); //2pb
    else if (type0 == 0x04) encryptedFrame[8] = random_n_to_m(11, 20); //4pb
    else if (type0 == 0x06) encryptedFrame[8] = random_n_to_m(21, 30); //6pb
    else if (type0 == 0x08) encryptedFrame[8] = random_n_to_m(31, 40); //8pb
    //IVA 6PB
    else if (type0 == 0x66) encryptedFrame[8] = random_n_to_m(41, 50); //6pb
    //ATLAS 3PB
    else if (type0 == 0x03) encryptedFrame[8] = random_n_to_m(51, 60); //ATLAS
    else if (type0 == 0x55) encryptedFrame[8] = random_n_to_m(61, 70); //Remote

    uint8_t type1 = originalFrame[9];
    uint8_t type2 = originalFrame[10];
    uint8_t type3 = originalFrame[11];

    // Type1 key generation
    if (type1 == 0x01) encryptedFrame[9] = random_n_to_m(1, 10);
    else if (type1 == 0x02) encryptedFrame[9] = random_n_to_m(11, 20);
    else if (type1 == 0x03) encryptedFrame[9] = random_n_to_m(21, 30);
    else if (type1 == 0x04) encryptedFrame[9] = random_n_to_m(31, 40);
    else if (type1 == 0x05) encryptedFrame[9] = random_n_to_m(41, 50);
    else if (type1 == 0x06) encryptedFrame[9] = random_n_to_m(51, 60);
    else if (type1 == 0x07) encryptedFrame[9] = random_n_to_m(61, 70);
    else if (type1 == 0x08) encryptedFrame[9] = random_n_to_m(71, 80);

//    emberAfCorePrintln("\nRandom key for type1 (%02X): %d\n", type1, encryptedFrame[8]);

    // Type2 key generation
    if (type2 == 0x01) encryptedFrame[10] = random_n_to_m(1, 20);
    else if (type2 == 0x02) encryptedFrame[10] = random_n_to_m(21, 40);
    else if (type2 == 0x03) encryptedFrame[10] = random_n_to_m(41, 60);
    else if (type2 == 0x04) encryptedFrame[10] = random_n_to_m(61, 80);
    else if (type2 == 0x0D) encryptedFrame[10] = random_n_to_m(81, 99);

   // emberAfCorePrintln("Random key for type2 (%02X): %d\n", type2, encryptedFrame[9]);

    // Type3 key generation
    if (type3 == 0x00) encryptedFrame[11] = random_n_to_m(1, 50);
    else if (type3 == 0x01) encryptedFrame[11] = random_n_to_m(51, 99);

   // emberAfCorePrintln("Random key for type3 (%02X): %d\n", type3, encryptedFrame[10]);

    encryptedFrame[12] = originalFrame[12] + 6;
    encryptedFrame[13] = originalFrame[13] - 3;

    GPIO_PinOutSet(WRITE_PORT, WRITE_PIN);
    GPIO_PinOutSet(READ_PORT, READ_PIN);

    emberAfCorePrintln("\nEncrypted Frame:\n");
    for (int i = 0; i < 14; i++) {
        USART_Tx(USART1, encryptedFrame[i]);
        emberAfCorePrint("%02X ", encryptedFrame[i]);
    }
    emberAfCorePrint("\n");

}

int hold_btn = 0;
bool btn1Hold = false, btn2Hold = false, btn3Hold = false,
    btn4Hold = false, btn5Hold = false, btn6Hold = false,
    btn1Press = false, btn2Press = false, btn3Press = false,
    btn4Press = false, btn5Press = false, btn6Press = false;//, btn7Press = false, btn8Press = false;
int btn1Cnt = 0,btn2Cnt = 0,btn3Cnt = 0,btn4Cnt = 0,btn5Cnt = 0,btn6Cnt = 0;//, btn7Cnt = 0, btn8Cnt = 0;
bool btn1LongHoldScheduled = false, btn2LongHoldScheduled = false, btn3LongHoldScheduled = false,
    btn4LongHoldScheduled = false, btn5LongHoldScheduled = false, btn6LongHoldScheduled = false;//, btn7LongHoldScheduled = false, btn8LongHoldScheduled = false;

void Button_Dimming_Handler()
{
    emberEventControlSetInactive(Button_Dimming_Control); // Stop repeated calls

    if (hold_btn == 1 && !btn1Hold)  // Only process long hold once per press
    {
        btn1Hold = true;
        emberAfCorePrintln("Button1 Long Hold");
        start_timer_for_button_presses(50);  // Call long hold action
    }
    else if (hold_btn == 2 && !btn2Hold)
    {
        btn2Hold = true;
        emberAfCorePrintln("Button2 Long Hold");
        start_timer_for_button_presses(60);
    }
    else if (hold_btn == 3 && !btn3Hold)
    {
        btn3Hold = true;
        emberAfCorePrintln("Button3 Long Hold");
        start_timer_for_button_presses(70);
    }
    else if (hold_btn == 4 && !btn4Hold)
    {
        btn4Hold = true;
        emberAfCorePrintln("Button4 Long Hold");
        start_timer_for_button_presses(80);
    }
    else if (hold_btn == 5 && !btn5Hold)
    {
        btn5Hold = true;
        emberAfCorePrintln("Button5 Long Hold");
        start_timer_for_button_presses(90);
    }
    else if (hold_btn == 6 && !btn6Hold)
    {
        btn6Hold = true;
        emberAfCorePrintln("Button6 Long Hold");
        start_timer_for_button_presses(100);
    }
    else if (hold_btn == 7 && !btn7Hold)
    {
        btn7Hold = true;
        emberAfCorePrintln("Button7 Long Hold");
        start_timer_for_button_presses(57); // Custom value for Button 7 long hold
    }

}

void Button_Isr_Poll_Handler()
{
  emberEventControlSetInactive(Button_Isr_Poll_Control);
  //emberAfCorePrintln("============In Button_Isr_Poll_Handler\n");
  static bool Button_1_State = 1;
  static bool Button_2_State = 1;
  static bool Button_3_State = 1;
  static bool Button_4_State = 1;
  static bool Button_5_State = 1;
  static bool Button_6_State = 1;
//  static bool Button_7_State = 1;
 // static bool Button_8_State = 1;

  if(Start_Up_Done == 0)
  {
      //Still in Start Up
      start_up_delay--;
      if(start_up_delay == 0)
      {
          //Start Up Delay Finished
          Start_Up_Done = 1;
          init_after_start_up_delay();
          Button_1_State = 1;
          Button_2_State = 1;
          Button_3_State = 1;
          Button_4_State = 1;
          Button_5_State = 1;
          Button_6_State = 1;
       //   Button_7_State = 1;
        //  Button_8_State = 1;
      }
      else
      {
        //Do not process button events
        emberEventControlSetDelayMS(Button_Isr_Poll_Control,BUTTON_POLL_TIME_MS);
        return;
      }
  }

  if(Button_1_State != GPIO_PinInGet(GPIO_BT1_PORT, Button_1))
  {
    Button_1_State = GPIO_PinInGet(GPIO_BT1_PORT, Button_1);
    if (Button_1_State == 0)  // Button Pressed
    {
        if (!btn1LongHoldScheduled) // Ensure event is scheduled only once
        {
            emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
            btn1LongHoldScheduled = true;
        }

        if (!btn1Hold)
        {
            btn1Press = true;
        }
        emberAfCorePrintln("Button1 Pressed");
        hold_btn = 1;
    }
    else
    {
      emberAfCorePrintln("Button1 Released");

    //  send_zigbee_button_event(1, C4_BUTTON_RELEASE_EVENT);
    //  set_led_when_button_event(1,0);
     // sendFrame(1);
//
     // set_relay_when_button_event(1,0);

    //  sendFrame(1);

//       char frame[20] = "#01011301000005";
//       GPIO_PinOutSet(WRITE_PORT,WRITE_PIN);
//       GPIO_PinOutSet(READ_PORT,READ_PIN);
//
//           int length = strlen(frame);
//           for (int i = 0; i < length; i++)
//           {
//               USART_Tx(USART1,frame[i]);
//               emberAfCorePrint("%c", frame[i]);
//           }
          // set_led(1,1);

      if (!btn1Hold && btn1Press)  // Short press action
      {
          start_timer_for_button_presses(1);

          if (btn1Cnt == 0)
          {
              btn1Cnt = 1;
//                 set_led(1, 1);
          }
          else if (btn1Cnt == 1)
          {
              btn1Cnt = 0;
//                 set_led(1, 0);
          }
      }

      // Reset flags only after release
      btn1Hold = false;
      btn1LongHoldScheduled = false; // Allow new long hold event for next press
      emberEventControlSetInactive(Button_Dimming_Control);

    }
  }

  if(Button_2_State != GPIO_PinInGet(GPIO_BT2_PORT, Button_2))
  {
    Button_2_State = GPIO_PinInGet(GPIO_BT2_PORT, Button_2);
    if(Button_2_State == 0)
    {
      emberAfCorePrintln("Button2 Pressed");
      if (!btn2LongHoldScheduled) // Ensure event is scheduled only once
      {
          emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
          btn2LongHoldScheduled = true;
      }
      if(!btn2Hold)
        {
          btn2Press = true;
        }
      emberAfCorePrintln("Button2 Pressed");
       hold_btn = 2;
    }
    else
    {
      emberAfCorePrintln("Button2 Released");

      if(!btn2Hold && btn2Press)
        {
          start_timer_for_button_presses(2);
          if(btn2Cnt == 0)
            {
              btn2Cnt = 1;
           //   set_led(2, 1);

            }
          else if(btn2Cnt == 1)
            {
              btn2Cnt = 0;
            //  set_led(2,0);

            }
        }
      btn2Hold = false;
      btn2LongHoldScheduled = false;
      emberEventControlSetInactive(Button_Dimming_Control);
    }
  }

  if(Button_3_State != GPIO_PinInGet(GPIO_BT3_PORT, Button_3))
  {
    Button_3_State = GPIO_PinInGet(GPIO_BT3_PORT, Button_3);
    if(Button_3_State == 0)
    {
        emberAfCorePrintln("Button3 Pressed");
        if (!btn3LongHoldScheduled) // Ensure event is scheduled only once
        {
            emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
            btn3LongHoldScheduled = true;
        }
        if(!btn3Hold)
          {
            btn3Press = true;
          }

        hold_btn = 3;

        emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
    }
    else
    {
      emberAfCorePrintln("Button3 Released");

      if(!btn3Hold && btn3Press)
         {

          start_timer_for_button_presses(3);
      if(btn3Cnt == 0)
        {
          btn3Cnt = 1;
         // set_led(3, 1);

        }
      else if(btn3Cnt == 1)
        {
          btn3Cnt = 0;
         // set_led(3,0);
        }
         }
      btn3Hold = false;
      btn3LongHoldScheduled = false;
      emberEventControlSetInactive(Button_Dimming_Control);
    }
  }

  if(Button_4_State != GPIO_PinInGet(GPIO_BT4_PORT, Button_4))
  {
    Button_4_State = GPIO_PinInGet(GPIO_BT4_PORT, Button_4);
    if(Button_4_State == 0)
    {
        emberAfCorePrintln("Button4 Pressed");
        if (!btn4LongHoldScheduled) // Ensure event is scheduled only once
        {
            emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
            btn4LongHoldScheduled = true;
        }
        if(!btn4Hold)
          {
            btn4Press = true;
          }

        hold_btn = 4;

        emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
    }
    else
    {
      emberAfCorePrintln("Button4 Released");

      if(!btn4Hold && btn4Press)
            {
          start_timer_for_button_presses(4);
         if(btn4Cnt == 0)
           {
             btn4Cnt = 1;

           //  set_led(4, 1);
           }
         else if(btn4Cnt == 1)
           {
             btn4Cnt = 0;
           //  set_led(4,0);

           }
            }
         btn4Hold = false;
         btn4LongHoldScheduled = false;
      emberEventControlSetInactive(Button_Dimming_Control);
    }
  }

  if(Button_5_State != GPIO_PinInGet(GPIO_BT5_PORT, Button_5))
  {
    Button_5_State = GPIO_PinInGet(GPIO_BT5_PORT, Button_5);
    if(Button_5_State == 0)
    {
        emberAfCorePrintln("Button5 Pressed");
        if (!btn5LongHoldScheduled) // Ensure event is scheduled only once
        {
            emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
            btn5LongHoldScheduled = true;
        }
        hold_btn = 5;
        if(!btn5Hold)
            {
              btn5Press = true;
            }

        emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
    }
    else
    {
      emberAfCorePrintln("Button5 Released");

      if(!btn5Hold && btn5Press)
    {
          start_timer_for_button_presses(5);
      if(btn5Cnt == 0)
        {
          btn5Cnt = 1;
         // set_led(5, 1);
        }
      else if(btn5Cnt == 1)
        {
          btn5Cnt = 0;
        //  set_led(5,0);

        }
    }
      btn5Hold = false;
      btn5LongHoldScheduled = false;
      emberEventControlSetInactive(Button_Dimming_Control);
    }
  }

  if(Button_6_State != GPIO_PinInGet(GPIO_BT6_PORT, Button_6))
  {
    Button_6_State = GPIO_PinInGet(GPIO_BT6_PORT, Button_6);
    if(Button_6_State == 0)
    {
        emberAfCorePrintln("Button6 Pressed");
        if (!btn6LongHoldScheduled) // Ensure event is scheduled only once
        {
            emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
            btn6LongHoldScheduled = true;
        }
        if(!btn6Hold)
             {
               btn6Press = true;
             }


        hold_btn = 6;


        emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
    }
    else
    {
      emberAfCorePrintln("Button6 Released");

      if(!btn6Hold && btn6Press)
        {
          start_timer_for_button_presses(6);
      if(btn6Cnt == 0)
        {
          btn6Cnt = 1;
         // set_led(6, 1);
        }
      else if(btn6Cnt == 1)
        {
          btn6Cnt = 0;
        //  set_led(6,0);

        }
     }
       btn6Hold = false;
       btn6LongHoldScheduled = false;
       emberEventControlSetInactive(Button_Dimming_Control);
    }
  }


//  if(Button_7_State != GPIO_PinInGet(APP_RELAY_7_PORT, APP_RELAY_7_PIN))
//  {
//    Button_7_State = GPIO_PinInGet(APP_RELAY_7_PORT, APP_RELAY_7_PIN);
//    if(Button_7_State == 0)
//    {
//      emberAfCorePrintln("Button7 Pressed");
//      start_timer_for_button_presses(7);
//
//      send_zigbee_button_event(7, C4_BUTTON_PRESS_EVENT);
//      set_led_when_button_event(7,1);
//      set_relay_when_button_event(7,1);
//    }
//    else
//    {
//      emberAfCorePrintln("Button7 Released");
//      send_zigbee_button_event(7, C4_BUTTON_RELEASE_EVENT);
//      set_led_when_button_event(7,0);
//      set_relay_when_button_event(7,0);
//    }
//  }
//
//  if(Button_8_State != GPIO_PinInGet(APP_RELAY_8_PORT, APP_RELAY_8_PIN))
//  {
//    Button_8_State = GPIO_PinInGet(APP_RELAY_8_PORT, APP_RELAY_8_PIN);
//    if(Button_8_State == 0)
//    {
//      emberAfCorePrintln("Button8 Pressed");
//      start_timer_for_button_presses(8);
//
//      send_zigbee_button_event(8, C4_BUTTON_PRESS_EVENT);
//      set_led_when_button_event(8,1);
//      set_relay_when_button_event(8,1);
//    }
//    else
//    {
//      emberAfCorePrintln("Button8 Released");
//      send_zigbee_button_event(8, C4_BUTTON_RELEASE_EVENT);
//      set_led_when_button_event(8,0);
//      set_relay_when_button_event(8,0);
//    }
//  }
 // uint8_t currentPin7State, currentPin8State;
  //MCP23017_2_GetPinStates(&currentPin7State, &currentPin8State);
  //emberAfCorePrintln("Initial Pin 7: %d, Initial Pin 8: %d\n", currentPin7State, currentPin8State);
  MCP23017_2_readReg(MCP23017_GPIO_A);
//  MCP23017_2_readReg(MCP23017_GPIO_B);

 // MCP23017_2_readReg(MCP23017_GPIO_B); // Read from Port B

  emberEventControlSetDelayMS(Button_Isr_Poll_Control,100);
}

void Sensor_interrupt_CB(uint8_t pin)
{
  emberAfCorePrintln("Sensor_interrupt_CB %d",pin);
  //emberAfCorePrintln("GPIO_PinInGet %d",GPIO_PinInGet(PORT_SENSOR_INT, PIN_SENSOR_INT));
  if(pin == 0)//Sensor Intr Pin is 0
  {
    if(!GPIO_PinInGet(PORT_SENSOR_INT, PIN_SENSOR_INT))
    {
     // emberAfCorePrintln("sensor_intr %d  HIGH",(!GPIO_PinInGet(PORT_SENSOR_INT, PIN_SENSOR_INT)));
      set_proximity_level();
    }
  }
}

void init_GPIOs()
{
  GPIOINT_Init();
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(GPIO_BT1_PORT, Button_1, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(GPIO_BT2_PORT, Button_2, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(GPIO_BT3_PORT, Button_3, gpioModeInputPullFilter, 1);
  //GPIO_PinModeSet(GPIO_BT4_PORT, Button_4, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_BT4_PORT, Button_4, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(GPIO_BT5_PORT, Button_5, gpioModeInputPullFilter, 1);
  //GPIO_PinModeSet(GPIO_BT6_PORT, Button_6, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_BT6_PORT, Button_6, gpioModeInputPullFilter, 1);
 // GPIO_PinModeSet(GPIO_BT7_PORT, Button_7, gpioModeInputPullFilter, 1);
 // GPIO_PinModeSet(GPIO_BT8_PORT, Button_8, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(DIM_PORT, DIM_PIN, gpioModeWiredAndPullUpFilter, OFF);

  GPIO_PinModeSet(PORT_SENSOR_INT, PIN_SENSOR_INT, gpioModeInputPull, 1);
  GPIO_ExtIntConfig(PORT_SENSOR_INT, PIN_SENSOR_INT, PIN_SENSOR_INT, true, true, true);
  GPIOINT_CallbackRegister(PIN_SENSOR_INT, Sensor_interrupt_CB);

//  GPIO_PinModeSet(GPIO_BT4_PORT, Button_4, gpioModePushPull, 0);
 // GPIO_PinModeSet(GPIO_BT5_PORT, Button_5, gpioModePushPull, 0);
//  GPIO_PinModeSet(GPIO_BT6_PORT, Button_6, gpioModePushPull, 0);
}

void emberAfMainInitCallback(void)
{
  //emberEventControlSetActive(closeNetworkEventControl);
  //Saving the data from flash memory of the Boot Count Cluster
  halCommonGetToken(&Boot_Count,TOKEN_BOOT_COUNT_196);
  Boot_Count++;
  int value[1]={Boot_Count};
  emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT_C4,
                                                  ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                  ZCL_BOOT_COUNT_ATTRIBUTE_ID,
                                                  EMBER_AF_MANUFACTURER_CODE,
                                                  (uint8_t*)value,
                                                  ZCL_INT32U_ATTRIBUTE_TYPE);
  emberAfCorePrintln("\n==============App Info===============");
  emberAfCorePrintln("Name       :CTSMZB_8PB3R_AC (%02d.%02d.%02d) ",MAIN_FW_VER-'0',MAJOR_BUG_FIX-'0',MINOR_BUG_FIX-'0');
  emberAfCorePrintln("Boot_Count :%d",Boot_Count);
  emberAfCorePrintln("=====================================");

  init_GPIOs();
  interface_iic_init();
  MCP23017_Init();
  MCP23017_2_Init();

  Dim_Init();
  Serial_Init();

  emberEventControlSetActive(Button_Isr_Poll_Control);

  EmberEUI64 macId;
  emberAfGetEui64(macId);  // Get the MAC ID
  for (int i = 0; i < 8; i++) {
      MACID[i] = macId[7 - i];
  }

//  emberAfCorePrint("MAC Bytes: ");
  for (int i = 0; i < 8; i++) {
//      emberAfCorePrint("%X", MACID[i]);

      btn1Buff[i] = MACID[i];
      btn2Buff[i] = MACID[i];
      btn3Buff[i] = MACID[i];
      btn4Buff[i] = MACID[i];
      btn5Buff[i] = MACID[i];
      btn6Buff[i] = MACID[i];
      btn7Buff[i] = MACID[i];
      btn8Buff[i] = MACID[i];
//      emberAfCorePrint("%X", btn6Buff[i]);
  }
  emberAfCorePrintln("");
}













void commissioningLedEventHandler(void)
{
    MCP23017_GPIO_PinOutToggle(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN);

    MCP23017_GPIO_PinOutToggle(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN);
    MCP23017_GPIO_PinOutToggle(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN);

    emberEventControlSetDelayMS(closeNetworkEventControl, 500);
}

/////////////////////////////////////////////////////////////////////////////////
/// Network Steering called 4 times with the delay of 10MS
/////////////////////////////////////////////////////////////////////////////////
//void Network_Steering_Event_Handler()
//{
//  emberEventControlSetInactive(Network_Steering_Event_Control);
//  emberEventControlSetInactive(closeNetworkEventControl);
//  emberAfCorePrintln("Network steer Timeout");
//  set_led_when_button_event(1,1);
//
//}


void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                       EmberAfAttributeId attributeId)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
  emberAfCorePrintln("emberAfOnOffClusterServerAttributeChangedCallback = %d attributeId = %d", endpoint, attributeId);
  if(Start_Up_Done == 0)
  {
    //Still in Start Up
    //Do NOT process further
    return;
  }
  if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
  {
    bool onOff;
    if (emberAfReadServerAttribute (endpoint,
                                    ZCL_ON_OFF_CLUSTER_ID,
                                    ZCL_ON_OFF_ATTRIBUTE_ID,
                                    (uint8_t*) &onOff, sizeof(onOff)) == EMBER_ZCL_STATUS_SUCCESS )
    {
      emberAfCorePrintln("Endpoint = %d onOff = %d", endpoint, onOff);
//      switch (endpoint)
//      {
//        case 1:
//          set_relay_when_zigbee_event(1,onOff);
//          set_led_when_zigbee_on_off_event(1,onOff);
//          emberEventControlSetDelayMS(app_Relay1StatusUpdateEventControl,100);
//        break;
//
//        case 2:
//          set_relay_when_zigbee_event(2,onOff);
//          set_led_when_zigbee_on_off_event(2,onOff);
//          emberEventControlSetDelayMS(app_Relay2StatusUpdateEventControl,100);
//        break;
//
//        case 3:
//          set_relay_when_zigbee_event(3,onOff);
//          set_led_when_zigbee_on_off_event(3,onOff);
//          emberEventControlSetDelayMS(app_Relay3StatusUpdateEventControl,100);
//        break;
//
//        default:
//
//        break;
//      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////
///        Starting the Broadcasting of the device to connect with network    ///
/// /////////////////////////////////////////////////////////////////////////////
void appBroadcastSearchTypeEventHandler()
{
  emberEventControlSetInactive(appBroadcastSearchTypeEventControl);
  uint8_t Radio_channel = emberAfGetRadioChannel ();

  EmberApsFrame emAfCommandApsFrame;
  emAfCommandApsFrame.profileId = 0xC25D;
  emAfCommandApsFrame.clusterId = 0x0001;
  emAfCommandApsFrame.sourceEndpoint = 0xC4;
  emAfCommandApsFrame.destinationEndpoint = 0xC4;
  emAfCommandApsFrame.options = EMBER_APS_OPTION_SOURCE_EUI64;

  uint8_t data[] =
  {
    0x18,                       //APS frame control
    Broadcast_seq,              // Sequence
    0x0A,                       // cmd(Report attributes)
    0x00, 0x00, 0x20, 0x03,
    0x04, 0x00,                 // FIRMWARE_VIRSION cmd
    0x42,                       // type (char string)
    0x08,                       // length (8 bit)
    0x30, MAIN_FW_VER, 0x2E, 0x30, MAJOR_BUG_FIX, 0x2E, 0x30, MINOR_BUG_FIX, // 01.00.00
    0x05, 0x00,                 // REFLASH_VERSION
    0x20,                       // type Unsigned 8-bit integer
    0x0FF,
    0x06, 0x00,                 // BOOT_COUNT
    0x21,                       // type Unsigned 16-bit integer
    Boot_Count & 0x00FF, Boot_Count >> 8,
    0x07, 0x00,                 // PRODUCT_STRING
    0x42,                       // type (Char String)
    0x0C,                       // Length
    0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x38, 0x50, 0x42, //(Confio:CT8PB)
    0x0c, 0x00,                 //MESH_CHANNEL
    0x20,
    Radio_channel
  };

  uint16_t dataLength = sizeof(data);
  EmberStatus status = emberAfSendBroadcastWithCallback (0xFFFC,
                                         &emAfCommandApsFrame,
                                         dataLength, data,
                                         NULL);
  emberAfCorePrintln(" ================Broadcast Status : 0x%02X \n",status);
  Broadcast_seq++;

}

bool  emberAfStackStatusCallback (EmberStatus status)
{
  emberAfCorePrintln(" ===========in status callback \n");
  if (status == EMBER_NETWORK_DOWN && emberAfNetworkState() == EMBER_NO_NETWORK)
  {
    //emAfPluginScenesServerClear();
    //emAfGroupsServerCliClear();
      emberAfCorePrintln(" ===========in status callback leave\n");
      emberEventControlSetDelayMS(Network_Steering_Event_Control, 2000);
  }

    if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
      {
  //         Network_open = false;
        emberAfCorePrintln(" ===========in status callback joined\n");
       // emberAfPluginNetworkCreatorSecurityOpenNetwork();
       // emberEventControlSetInactive(closeNetworkEventControl);
      //  makeLedToDefault();
       // emberEventControlSetDelayMS(appBroadcastSearchTypeCT4REventControl,500);
       // emberEventControlSetDelayMS(app4RAnnouncementEventControl,2000);
  //         networkJoin = true;
      }
  return false;
}

bool emberAfPluginScenesServerCustomRecallSceneCallback (
    const EmberAfSceneTableEntry *const sceneEntry,
    uint16_t transitionTimeDs,
    EmberAfStatus *const status)
{
  //Relay_Operation(sceneEntry->endpoint,sceneEntry->onOffValue);
  return true;
}




/** @brief custom end device configuration Cluster Cluster Server Manufacturer Specific Attribute Changed
 *
 * Server Manufacturer Specific Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 * @param manufacturerCode Manufacturer Code of the attribute that changed
 * Ver.: always
 */
void emberAfCustomEndDeviceConfigurationClusterServerManufacturerSpecificAttributeChangedCallback(int8u endpoint,
                                                                                                  EmberAfAttributeId attributeId,
                                                                                                  int16u manufacturerCode)
{
  if(Start_Up_Done == 0)
  {
    //Still in Start Up
    //Do NOT process further
    return;
  }
  if (attributeId == ZCL_C4_LED_1_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_1_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_1,
    sizeof(C4_Led_1));
   // emberAfCorePrintln("C4_Led_1                  %d",C4_Led_1);


  }
  else if (attributeId == ZCL_C4_LED_2_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_2_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_2,
    sizeof(C4_Led_2));
   // emberAfCorePrintln("C4_Led_2                  %d",C4_Led_2);


  }
  else if (attributeId == ZCL_C4_LED_3_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_3_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_3,
    sizeof(C4_Led_3));
   // emberAfCorePrintln("C4_Led_3                  %d",C4_Led_3);

  }
  else if (attributeId == ZCL_C4_LED_4_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_4_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_4,
    sizeof(C4_Led_4));
   // emberAfCorePrintln("C4_Led_4                  %d",C4_Led_4);

  }
  else if (attributeId == ZCL_C4_LED_5_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_5_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_5,
    sizeof(C4_Led_5));
  //  emberAfCorePrintln("C4_Led_5                  %d",C4_Led_5);


  }
  else if (attributeId == ZCL_C4_LED_6_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_6_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_6,
    sizeof(C4_Led_6));
   // emberAfCorePrintln("C4_Led_6                  %d",C4_Led_6);

  }
  else if (attributeId == ZCL_C4_LED_7_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_7_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_7,
    sizeof(C4_Led_7));
   // emberAfCorePrintln("C4_Led_7                  %d",C4_Led_7);


  }
  else if (attributeId == ZCL_C4_LED_8_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_8_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_8,
    sizeof(C4_Led_8));
   // emberAfCorePrintln("C4_Led_8                  %d",C4_Led_8);


  }


  else if (attributeId == ZCL_C4_BUTTON_1_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_1_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_1,
    sizeof(C4_Button_Relay_1));
   // emberAfCorePrintln("C4_Button_Relay_1        %d",C4_Button_Relay_1);


  }
  else if (attributeId == ZCL_C4_BUTTON_2_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_2_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_2,
    sizeof(C4_Button_Relay_2));
   // emberAfCorePrintln("C4_Button_Relay_2        %d",C4_Button_Relay_2);


  }
  else if (attributeId == ZCL_C4_BUTTON_3_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_3_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_3,
    sizeof(C4_Button_Relay_3));
  //  emberAfCorePrintln("C4_Button_Relay_3        %d",C4_Button_Relay_3);


  }
  else if (attributeId == ZCL_C4_BUTTON_4_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_4_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_4,
    sizeof(C4_Button_Relay_4));
   // emberAfCorePrintln("C4_Button_Relay_4        %d",C4_Button_Relay_4);


  }
  else if (attributeId == ZCL_C4_BUTTON_5_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_5_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_5,
    sizeof(C4_Button_Relay_5));
   // emberAfCorePrintln("C4_Button_Relay_5        %d",C4_Button_Relay_5);


  }
  else if (attributeId == ZCL_C4_BUTTON_6_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_6_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_6,
    sizeof(C4_Button_Relay_6));
   // emberAfCorePrintln("C4_Button_Relay_6        %d",C4_Button_Relay_6);


  }
  else if (attributeId == ZCL_C4_BUTTON_7_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_7_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_7,
    sizeof(C4_Button_Relay_7));
   // emberAfCorePrintln("C4_Button_Relay_7        %d",C4_Button_Relay_7);


  }
  else if (attributeId == ZCL_C4_BUTTON_8_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_BUTTON_8_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Button_Relay_8,
    sizeof(C4_Button_Relay_8));
   // emberAfCorePrintln("C4_Button_Relay_8        %d",C4_Button_Relay_8);


  }

  else if (attributeId == ZCL_C4_LED_1_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_1_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
   // emberAfCorePrintln("follow_connection       1 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_2_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_2_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
  //  emberAfCorePrintln("follow_connection       2 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_3_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_3_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
    //emberAfCorePrintln("follow_connection       3 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_4_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_4_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
   // emberAfCorePrintln("follow_connection       4 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_5_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_5_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
  //  emberAfCorePrintln("follow_connection       5 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_6_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_6_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
    //emberAfCorePrintln("follow_connection       6 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_7_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_7_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
    //emberAfCorePrintln("follow_connection       7 %d",temp);


  }
  else if (attributeId == ZCL_C4_LED_8_STATE_ATTRIBUTE_ID)
  {
    uint8_t temp = 0;
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_8_STATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&temp,
    sizeof(temp));
   // emberAfCorePrintln("follow_connection       8 %d",temp);


  }

  else if (attributeId == ZCL_C4_LED_DIM_UP_RATE_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_DIM_UP_RATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_Dim_Up_Rate,
    sizeof(C4_Led_Dim_Up_Rate));
    //emberAfCorePrintln("C4_Led_Dim_Up_Rate        %d",C4_Led_Dim_Up_Rate);
  }
  else if (attributeId == ZCL_C4_LED_DIM_DOWN_RATE_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_DIM_DOWN_RATE_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_Dim_Down_Rate,
    sizeof(C4_Led_Dim_Down_Rate));
   // emberAfCorePrintln("C4_Led_Dim_Down_Rate      %d",C4_Led_Dim_Down_Rate);
  }
  else if (attributeId == ZCL_C4_LED_DIM_MIN_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_DIM_MIN_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_Dim_Min,
    sizeof(C4_Led_Dim_Min));
    //emberAfCorePrintln("C4_Led_Dim_Min            %d",C4_Led_Dim_Min);
  }
  else if (attributeId == ZCL_C4_LED_DIM_MAX_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_DIM_MAX_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_Dim_Max,
    sizeof(C4_Led_Dim_Max));
    //emberAfCorePrintln("C4_Led_Dim_Max            %d",C4_Led_Dim_Max);
  }
  else if (attributeId == ZCL_C4_LED_ON_TIME_ATTRIBUTE_ID)
  {
    emberAfReadManufacturerSpecificServerAttribute(ENDPOINT_C4,
    ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
    ZCL_C4_LED_ON_TIME_ATTRIBUTE_ID,
    EMBER_AF_MANUFACTURER_CODE,
    (uint8_t *)&C4_Led_On_Time,
    sizeof(C4_Led_On_Time));
    emberAfCorePrintln("C4_Led_On_Time2            %d",C4_Led_On_Time);
  }
  else
  {
      //emberAfCorePrintln("ManufacturerSpecificAttributeChangedCallback %X %X %2X",endpoint,attributeId, manufacturerCode);
      //emberAfCorePrintln("NOT handled");
  }
}




//Payload: 54 0F 57 FF FE 0A BC C3 00 C4 21 01 01 */
boolean emberAfPreCommandReceivedCallback (EmberAfClusterCommand *cmd)
{
// // emberAfCorePrintln("====emberAfPreCommandReceivedCallback===");
//
//  // Print the Cluster ID
// // emberAfAppPrintln("Cluster ID: %X", cmd->apsFrame->clusterId);
//
//  // Print the payload
//  emberAfAppPrint("Payload: ");
//  for (uint8_t i = cmd->payloadStartIndex; i < cmd->bufLen; i++) {
//      emberAfAppPrint("%d-%X ", i, cmd->buffer[i]);
//  }
//  emberAfAppPrintln("");

  if(cmd->apsFrame->clusterId == 0xFC00)
    {
      for (uint8_t i = cmd->payloadStartIndex; i < 11; i++) //54 0F 57 FF FE 0A BC C3 -MAC ID
       {
          //emberAfAppPrint("i-%X ", i, cmd->buffer[i]);
          Remote_Data[i - 3] = cmd->buffer[i];
      }
      Remote_Data[8] = 0x55;//REMOTE
      Remote_Data[9] = cmd->buffer[15];
      Remote_Data[10] = cmd->buffer[14];
      Remote_Data[11] = 1;
      uint16_t crc = calculate_modbus_crc(Remote_Data, 12);

      Remote_Data[12] = crc & 0xFF;
      Remote_Data[13] = (crc >> 8) & 0xFF;
      emberAfAppPrintln("--Remote Final Data--");
      for (int i = 0; i < 14; i++) {
          emberAfCorePrint("%X ", Remote_Data[i]);
      }
      emberAfCorePrintln("\n");
    /*--Remote Final Data--
    54 0F 57 FF FE 0A BC C3 06 01 01 FF F8 CB */ //BTN1-SP
      uint32_t encryptedFrame[14];
      encryptFrameandSend(Remote_Data, encryptedFrame);
    }
  return false;
}






