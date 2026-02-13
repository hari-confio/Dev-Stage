/*
 * App_Main.c
 *
 *  Created on: 10-Jan-2023
 *      Author: Confio
 */


/*
Button 1 (Long Press) to include to the network

Button 1 (13 Times press) to exclude from the network
*/


#include "App_Scr/App_Main.h"
#include "em_timer.h"
#include "em_usart.h"
#include "MCP23017_I2C.h"
#include "gpiointerrupt.h"
#include "em_gpio.h"
#include "em_core.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "nvm3.h"
#include "nvm3_default.h"
#include "app/framework/plugin/network-creator-security/network-creator-security.h"
#include "app/framework/plugin/network-creator/network-creator.h"

uint8_t Led1_Count = 0;
uint8_t Led2_Count = 0;
uint8_t Led3_Count = 0;
uint8_t Led4_Count = 0;
uint8_t Led5_Count = 0;
uint8_t Led6_Count = 0;

uint16_t Boot_Count = 0;
uint8_t Broadcast_seq = 1;
bool networkJoin = false;

bool Debounce_Identity = false;
//bool Network_Steer;

EmberEventControl closeNetworkEventControl;
EmberEventControl Switch_Debounce_Event_Control;
EmberEventControl Button_Isr_Poll_Control;
EmberEventControl appBroadcastSearchTypeCT4REventControl;
EmberEventControl Network_Steering_Event_Control;
EmberEventControl makeLedsToDefault_Control;
EmberEventControl Button_Dimming_Control;

//uint8_t Button_1_Count = 0;
void processReceivedData();
void sendFrame(uint8_t *data, size_t dataSize);
void encryptAndSendData(uint32_t *data, int length);
void  led_set_to_white();
void set_led(int button, int state);
void create_frame(int button, int action_type);
int ledBlinkCnt = 0;
void led_set_to_white();
void led_set_to_orange();
void Sensor_interrupt_CB(uint8_t pin);
//bool ledState11 = false, ledState21 = false, ledState31 = false, ledState41 = false, ledState51 = false, ledState61 = false;  // Toggle for active_count == 1
//bool ledState12 = false, ledState22 = false, ledState32 = false, ledState42 = false, ledState52 = false, ledState62 = false;  // Toggle for active_count == 2
//bool ledState1Long = false, ledState2Long = false, ledState3Long = false, ledState4Long = false, ledState5Long = false, ledState6Long = false;  // Toggle for long hold

#include EMBER_AF_API_NETWORK_CREATOR
#include EMBER_AF_API_NETWORK_CREATOR_SECURITY
#include EMBER_AF_API_NETWORK_STEERING
#include EMBER_AF_API_FIND_AND_BIND_TARGET

#define RX_BUFF_LEN 64
volatile uint16_t txFrameLength;
uint8_t txSum = 0;
volatile uint16_t Rx_Buff[RX_BUFF_LEN] = {0};
 uint32_t Remote_Data[RX_BUFF_LEN] = {0};

static uint16_t Rx_Buff_Len = 0;
#define RX_BUFF_SIZE 128


void closeNetworkEventHandler() {
  emberEventControlSetInactive(closeNetworkEventControl);
  emberAfPluginNetworkCreatorSecurityCloseNetwork();
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

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);

    uint32_t data;

    // Start a timer (HW timer)
    TIMER_CounterSet(TIMER1, 1);
    TIMER_Enable(TIMER1, true);

    data = (uint32_t)USART_Rx(USART2);
  //  emberAfCorePrintln("%02X ", data);
    if (Rx_Buff_Len < RX_BUFF_SIZE) // Ensure no overflow
    {
        Rx_Buff[Rx_Buff_Len++] = data;
       // emberAfCorePrintln("%02X ", data);
    }
    else
    {
        emberAfCorePrintln("Rx_Buff overflow");
    }
}

void TIMER1_IRQHandler(void)
{

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
        if(mac_match_cnt == 7 && Rx_Buff[8] == 0x66) //0x66 Indicates 6pb IVA
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
                     set_led(1, Rx_Buff[11]);
//                     emberAfCorePrintln("--Response--BTN - %d\n", 1);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState12 = !ledState12;  // Toggle LED state for Button == 1 & DoublePress
                     set_led(1, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                   //  ledState1Long = !ledState1Long;  // Toggle LED state for for Button == 1 & long hold
                     set_led(1, Rx_Buff[11]);
                 }
              }
            else if(Rx_Buff[9] == 0x02)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState21 = !ledState21;
                     set_led(2, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState22 = !ledState22;
                     set_led(2, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                   //  ledState2Long = !ledState2Long;
                     set_led(2, Rx_Buff[11]);
                 }
              }
            else if(Rx_Buff[9] == 0x03)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState31 = !ledState31;
                     set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState32 = !ledState32;
                     set_led(3, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState3Long = !ledState3Long;
                     set_led(3, Rx_Buff[11]);
                 }
              }
            else if(Rx_Buff[9] == 0x04)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState41 = !ledState41;
                     set_led(4, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState42 = !ledState42;
                     set_led(4, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState4Long = !ledState4Long;
                     set_led(4, Rx_Buff[11]);
                 }
              }
            else if(Rx_Buff[9] == 0x05)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                   //  ledState51 = !ledState51;
                     set_led(5, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                    // ledState52 = !ledState52;
                     set_led(5, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState5Long = !ledState5Long;
                     set_led(5, Rx_Buff[11]);
                 }
              }
            else if(Rx_Buff[9] == 0x06)
              {
                 if(Rx_Buff[10] == 0x01)
                 {
                    // ledState61 = !ledState61;
                     set_led(6, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x02)
                 {
                   // ledState62 = !ledState62;
                     set_led(6, Rx_Buff[11]);
                 }
                 else if(Rx_Buff[10] == 0x03)
                 {
                    // ledState6Long = !ledState6Long;
                     set_led(6, Rx_Buff[11]);
                 }
              }
      }

      }

  }


    memset(Rx_Buff, 0, sizeof(Rx_Buff)); // Clear buffer
    Rx_Buff_Len = 0; // Reset length

}
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

//void commissioningLedEventHandler(void)
//{
//
//
//
//
//  emberEventControlSetDelayMS(closeNetworkEventControl, 500);
////  ledBlinkCnt++;
////  if(ledBlinkCnt == 6) // Toggle 3 times
////    {
////      emberEventControlSetInactive(closeNetworkEventControl);
////      ledBlinkCnt = 0;
////    }
//}


//int scene_value = 0;
int btnSceneLed = 0;
uint8_t active_button = 0;
uint8_t active_count = 0;


  void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                   bool usedSecondaryChannels)
  {
    emberAfCorePrintln("%p network %p: 0x%X",
                       "Form distributed",
                       "complete",
                       EMBER_SUCCESS);
  }


uint32_t btn1Buff[14], btn2Buff[14], btn3Buff[14], btn4Buff[14], btn5Buff[14], btn6Buff[14];

void Switch_Counter_Handler()
{
  emberEventControlSetInactive(Switch_Counter_Control);
  emberAfCorePrintln("active_button  %d %d", active_button, active_count);
  set_proximity_level();
  if(active_button== 1 && active_count == 4)
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
      emberEventControlSetDelayMS(makeLedsToDefault_Control , 2000);
      btn1Buff[11] = 1;
      create_frame(1, active_count);

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

//          ledState11 = !ledState11;  // Toggle LED state for count == 1
//          set_led(1, ledState11);
          btn1Buff[11] = 1;//ledState11;
          create_frame(1, active_count);
      }
      else if (active_count == 2)
      {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

//          ledState12 = !ledState12;  // Toggle LED state for count == 2
//          set_led(1, ledState12);
          btn1Buff[11] = 1;//ledState12;
          create_frame(1, active_count);
      }
      else if (active_button == 50) // 50 indicates button long hold action
      {
          emberAfCorePrintln("---> Button %d Long Held\n", 1);

//          ledState1Long = !ledState1Long;  // Toggle LED state for long hold
//          set_led(1, ledState1Long);
          btn1Buff[11] = 1;//ledState1Long;
          create_frame(1, 3);
      }


  }

  else if(active_button == 2 || active_button == 60)
    {
      if(active_button == 2 && active_count == 1)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
         // ledState21 = !ledState21;
        //  set_led(2, ledState21);
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
        //  ledState2Long = !ledState2Long;
        //  set_led(2, ledState2Long);
          btn2Buff[11] = 1;//ledState2Long;
          create_frame(2, 3);
        }
    }

  else if(active_button == 3 || active_button == 70)
    {
      if(active_button == 3 && active_count == 1)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
        //  ledState31 = !ledState31;
        //  set_led(3, ledState31);
          btn3Buff[11] = 1;//ledState31;
          create_frame(3, active_count);
        }
      else if(active_count == 2)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
         // ledState32 = !ledState32;
        //  set_led(3, ledState32);
          btn3Buff[11] = 1;//ledState32;
          create_frame(3, active_count);
        }
      else if(active_button == 70)
        {
          emberAfCorePrintln("---> Button %d Long Held\n", 3);
        //  ledState3Long = !ledState3Long;
        //  set_led(3, ledState3Long);
          btn3Buff[11] = 1;//ledState3Long;
          create_frame(3, 3);
        }
    }
  else if(active_button == 4 || active_button == 80)
    {
      if(active_button == 4 && active_count == 1)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
         // ledState41 = !ledState41;
        //  set_led(4, ledState41);
          btn4Buff[11] = 1;//ledState41;
          create_frame(4, active_count);
        }
      else if(active_count == 2)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
       //   ledState42 = !ledState42;
        //  set_led(4, ledState42);
          btn4Buff[11] = 1;//ledState42;
          create_frame(4, active_count);
        }
      else if(active_button == 80)
        {
          emberAfCorePrintln("---> Button %d Long Held\n", 4);
         // ledState4Long = !ledState4Long;
        //  set_led(4, ledState4Long);
          btn4Buff[11] = 1;//ledState4Long;
          create_frame(4, 3);
        }
    }
  else if(active_button == 5 || active_button == 90)
    {
      if(active_button == 5 && active_count == 1)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
        //  ledState51 = !ledState51;
        //  set_led(5, ledState51);
          btn5Buff[11] = 1;//ledState51;
          create_frame(5, active_count);
        }
      else if(active_count == 2)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
        //  ledState52 = !ledState52;
        //  set_led(5, ledState52);
          btn5Buff[11] = 1;//ledState52;
          create_frame(5, active_count);
        }
      else if(active_button == 90)
        {
          emberAfCorePrintln("---> Button %d Long Held\n", 5);
        //  ledState5Long = !ledState5Long;
        //  set_led(5, ledState5Long);
          btn5Buff[11] = 1;//ledState5Long;
          create_frame(5, 3);
        }
    }
  else if(active_button == 6 || active_button == 100)
    {
      if(active_button == 6 && active_count == 1)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
         // ledState61 = !ledState61;
        //  set_led(6, ledState61);
          btn6Buff[11] = 1;//ledState61;
          create_frame(6, active_count);
        }
      else if(active_count == 2)
        {
          emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);
         // ledState62 = !ledState62;
        //  set_led(6, ledState62);
          btn6Buff[11] = 1;//ledState62;
          create_frame(6, active_count);
        }
      else if(active_button == 100)
        {
          emberAfCorePrintln("---> Button %d Long Held\n", 6);
         // ledState6Long = !ledState6Long;
        //  set_led(6, ledState6Long);
          btn6Buff[11] = 1;//ledState6Long;
          create_frame(6, 3);
        }
    }

  active_button = 0;
  active_count = 0;
}


void  button_presses(uint8_t button)
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


void set_led(int button, int state)
{
  set_proximity_level();
  if(state) // Yellow LED
    {

    if(button == 1)
      {
         MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 0);
         MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 1);
      }
    if(button == 2)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 0);
        MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 1);
      }
    if(button == 3)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 0);
        MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 1);

      }
    if(button == 4)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 0);
        MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 1);

      }
    if(button == 5)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 0);
        MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 1);

      }
    if(button == 6)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 0);
        MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 1);

      }
    }
  if(!state) // White LED
    {

    if(button == 1)
      {
         MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
         MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);

      }
    if(button == 2)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
        MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);

      }
    if(button == 3)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
        MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);

      }
    if(button == 4)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
        MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);

      }
    if(button == 5)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
        MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);

      }
    if(button == 6)
      {
        MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
        MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);

      }
    }
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

  activeBuff[8] = KEYPAD_TYPE; //IVA 6pb
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
    if      (val11 >= 1  && val11 <= 50) frame[11] = 0x00;//On
    else if (val11 >= 51 && val11 <= 99) frame[11] = 0x01;//Off
    else if (val11 >= 120) frame[11] = add_sub_transform(frame[11], -120);//ATLAS-Dimming

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
    else if (type0 == 0x66) encryptedFrame[8] = random_n_to_m(41, 50); //IVA 6pb
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
    if (type3 == 0x00) encryptedFrame[11] = random_n_to_m(1, 50); //On
    else if (type3 == 0x01) encryptedFrame[11] = random_n_to_m(51, 99); //Off
    else encryptedFrame[11] = originalFrame[11] + 120; //Atlas-Dimming

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
    btn4Press = false, btn5Press = false, btn6Press = false;
int btn1Cnt = 0,btn2Cnt = 0,btn3Cnt = 0,btn4Cnt = 0,btn5Cnt = 0,btn6Cnt = 0;
bool btn1LongHoldScheduled = false, btn2LongHoldScheduled = false, btn3LongHoldScheduled = false,
    btn4LongHoldScheduled = false, btn5LongHoldScheduled = false, btn6LongHoldScheduled = false;


void Button_Dimming_Handler()
{
    emberEventControlSetInactive(Button_Dimming_Control); // Stop repeated calls

    if (hold_btn == 1 && !btn1Hold)  // Only process long hold once per press
    {
        btn1Hold = true;
        emberAfCorePrintln("Button1 Long Hold");
        button_presses(50);  // Call long hold action
    }
    else if (hold_btn == 2 && !btn2Hold)
    {
        btn2Hold = true;
        emberAfCorePrintln("Button2 Long Hold");
        button_presses(60);
    }
    else if (hold_btn == 3 && !btn3Hold)
    {
        btn3Hold = true;
        emberAfCorePrintln("Button3 Long Hold");
        button_presses(70);
    }
    else if (hold_btn == 4 && !btn4Hold)
    {
        btn4Hold = true;
        emberAfCorePrintln("Button4 Long Hold");
        button_presses(80);
    }
    else if (hold_btn == 5 && !btn5Hold)
    {
        btn5Hold = true;
        emberAfCorePrintln("Button5 Long Hold");
        button_presses(90);
    }
    else if (hold_btn == 6 && !btn6Hold)
    {
        btn6Hold = true;
        emberAfCorePrintln("Button6 Long Hold");
        button_presses(100);
    }
}



void Button_Isr_Poll_Handler()
{
  emberEventControlSetInactive(Button_Isr_Poll_Control);

//  emberAfCorePrintln("============In Button_Isr_Poll_Handler\n");
 static bool Button_1_State = Released;
 static bool Button_2_State = Released;
 static bool Button_3_State = Released;
 static bool Button_4_State = Released;
 static bool Button_5_State = Released;
 static bool Button_6_State = Released;

 if (Button_1_State != GPIO_PinInGet(GPIO_BT1_PORT, Button_1))
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
     else  // Button Released
     {
         emberAfCorePrintln("Button1 Released");
         if (!btn1Hold && btn1Press)  // Short press action
         {
             button_presses(1);

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

 if(Button_2_State != GPIO_PinInGet (GPIO_BT2_PORT, Button_2))
   {
     Button_2_State = GPIO_PinInGet (GPIO_BT2_PORT, Button_2);
     if(Button_2_State == 0)
       {
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
             button_presses(2);
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

 if(Button_3_State != GPIO_PinInGet (GPIO_BT3_PORT, Button_3))
   {
     Button_3_State = GPIO_PinInGet (GPIO_BT3_PORT, Button_3);
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

         button_presses(3);
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

 if(Button_4_State != GPIO_PinInGet (GPIO_BT4_PORT, Button_4))
   {
     Button_4_State = GPIO_PinInGet (GPIO_BT4_PORT, Button_4);
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
         button_presses(4);
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

 if(Button_5_State != GPIO_PinInGet (GPIO_BT5_PORT, Button_5))
   {
     Button_5_State = GPIO_PinInGet (GPIO_BT5_PORT, Button_5);
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
         button_presses(5);
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

 if(Button_6_State != GPIO_PinInGet (GPIO_BT6_PORT, Button_6))
   {
     Button_6_State = GPIO_PinInGet (GPIO_BT6_PORT, Button_6);
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
         button_presses(6);
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
 MCP23017_readReg(MCP23017_GPIO_A);
  emberEventControlSetDelayMS(Button_Isr_Poll_Control,100);

}


//void read_NVM_data()
//{
//  Ecode_t status;
//  status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B1SP, Remote_Btn1SP_Data, sizeof(Remote_Btn1SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B1SP read -- FAIL--");
//  }
//  status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B1DP, Remote_Btn1DP_Data, sizeof(Remote_Btn1DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B1DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B1LH, Remote_Btn1LH_Data, sizeof(Remote_Btn1LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B1LH read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B2SP, Remote_Btn2SP_Data, sizeof(Remote_Btn2SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B2SP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B2DP, Remote_Btn2DP_Data, sizeof(Remote_Btn2DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B2DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B2LH, Remote_Btn2LH_Data, sizeof(Remote_Btn2LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B2LH read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B3SP, Remote_Btn3SP_Data, sizeof(Remote_Btn3SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B3SP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B3DP, Remote_Btn3DP_Data, sizeof(Remote_Btn3DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B3DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B3LH, Remote_Btn3LH_Data, sizeof(Remote_Btn3LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B3LH read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B4SP, Remote_Btn4SP_Data, sizeof(Remote_Btn4SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B4SP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B4DP, Remote_Btn4DP_Data, sizeof(Remote_Btn4DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B4DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B4LH, Remote_Btn4LH_Data, sizeof(Remote_Btn4LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B4LH read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B5SP, Remote_Btn5SP_Data, sizeof(Remote_Btn5SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B5SP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B5DP, Remote_Btn5DP_Data, sizeof(Remote_Btn5DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B5DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B5LH, Remote_Btn5LH_Data, sizeof(Remote_Btn5LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B5LH read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B6SP, Remote_Btn6SP_Data, sizeof(Remote_Btn6SP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B6SP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B6DP, Remote_Btn6DP_Data, sizeof(Remote_Btn6DP_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B6DP read -- FAIL--");
//  }
//   status = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_B6LH, Remote_Btn6LH_Data, sizeof(Remote_Btn6LH_Data));
//  if (status != ECODE_OK) {
//      emberAfCorePrintln("--NVM3_KEY_B6LH read -- FAIL--");
//  }
//
//}

/** @brief Server Attribute Changed
 *
 * On/off cluster, Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */

/////////////////////////////////////////////////////////////////////////////////
///     When particular switch get data to off Led and work with other device ///
/////////////////////////////////////////////////////////////////////////////////
void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                       EmberAfAttributeId attributeId)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
 if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
     {
       bool onOff;

       if (emberAfReadServerAttribute (endpoint,
                                       ZCL_ON_OFF_CLUSTER_ID,
                                       ZCL_ON_OFF_ATTRIBUTE_ID,
                                       (uint8_t*) &onOff, sizeof(onOff))
           == EMBER_ZCL_STATUS_SUCCESS )
         {

         }
     }
}

/////////////////////////////////////////////////////////////////////////////////
///        Starting the Broadcasting of the device to connect with network    ///
/// /////////////////////////////////////////////////////////////////////////////
void appBroadcastSearchTypeCT4REventHandler()
{
  emberEventControlSetInactive(appBroadcastSearchTypeCT4REventControl);

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
//        0x0B,                       // Length
//        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x34, 0x52, //(Confio:CT4R)
//        0x0E, //Length

//        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x4D, 0x54, 0x53, 0x34, 0x52, //(Confio:CTMTS4R)

        //  0x0C,                       // Length
        //  0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x36, 0x50, 0x42, //(Confio:CT6PB)
          0x11,                       // Length
          0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x36, 0x50, 0x42, 0x52, 0x53, 0x34, 0x38, 0x35,//(Confio:CT6PBRS485)

          0x0c, 0x00,  //MESH_CHANNEL
          0x20,
          Radio_channel, };

  uint16_t dataLength = sizeof(data);
  EmberStatus status = emberAfSendBroadcastWithCallback (0xFFFC,
                                                         &emAfCommandApsFrame,
                                                         dataLength, data,
                                                         NULL);
  emberAfCorePrintln(" =======new = 4R =========Broadcast Status : 0x%02X \n",status);
  Broadcast_seq++;

}


EmberEventControl findingAndBindingEventControl;
void findingAndBindingEventHandler()
{
  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    emberEventControlSetInactive(findingAndBindingEventControl);
    emberAfCorePrintln("Find and bind target start: 0x%X",
                       emberAfPluginFindAndBindTargetStart(1));
  }
}

//void  led_set_to_white()
//{
//MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
//MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
//
//MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
//MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
//}

bool  emberAfStackStatusCallback (EmberStatus status)
{
  emberAfCorePrintln(" ===========in status callback \n");
      if (status == EMBER_NETWORK_DOWN && emberAfNetworkState() == EMBER_NO_NETWORK)
       {
//         emAfPluginSCENEServerClear();
//         emAfGroupsServerCliClear();
          emberAfCorePrintln(" ===========in status callback leave\n");
         // emberEventControlSetActive(closeNetworkEventControl);
          emberEventControlSetDelayMS(Network_Steering_Event_Control, 2000);

         // networkJoin = false;
         // nvm3_eraseAll(nvm3_defaultHandle);
       }

     if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
       {
//         Network_open = false;
         emberAfCorePrintln(" ===========in status callback joined\n");
       //  emberAfPluginNetworkCreatorSecurityOpenNetwork();
        // emberEventControlSetDelayMS(appBroadcastSearchTypeCT4REventControl,500);
        // emberEventControlSetDelayMS(app4RAnnouncementEventControl,2000);
         //emberEventControlSetInactive(closeNetworkEventControl);
         //makeLedToDefault();
//         networkJoin = true;
       }
//     if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
//       {
////         Network_open = false;
//         emberAfCorePrintln(" ===========in status callback joined 1\n");
//       //  emberAfPluginNetworkCreatorSecurityOpenNetwork();
//        // emberEventControlSetInactive(closeNetworkEventControl);
//       //  makeLedToDefault();
//        // emberEventControlSetDelayMS(appBroadcastSearchTypeCT4REventControl,500);
//        // emberEventControlSetDelayMS(app4RAnnouncementEventControl,2000);
////         networkJoin = true;
//       }


  return false;
}

bool emberAfPluginSCENEServerCustomRecallSceneCallback (
    const EmberAfSceneTableEntry *const sceneEntry, uint16_t transitionTimeDs,
    EmberAfStatus *const status)
{
//      Relay_Operation(sceneEntry->endpoint,sceneEntry->onOffValue);
  return true;
}

//emberPermitJoining(180)
/////////////////////////////////////////////////////////////////////////////////
///  This Callback is called when any data is received from the server        ///
/////////////////////////////////////////////////////////////////////////////////
/*
T00000000:RX len 16, ep 01, clus 0xFC00 (Unknown clus. [0xFC00]) FC 08 seq 33 cmd 0A payload[54 0F 57 FF FE 0A BC C3 00 C4 21 01 01 ]
====emberAfPreCommandReceivedCallback===
Cluster ID: 0x0000FC00
Payload: 54 0F 57 FF FE 0A BC C3 00 C4 21 01 01 */
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



/** @brief custom end device configuration Cluster Cluster Server Pre Attribute Changed
 *
 * Server Pre Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute to be changed  Ver.: always
 * @param attributeType Attribute type  Ver.: always
 * @param size Attribute size  Ver.: always
 * @param value Attribute value  Ver.: always
 */
EmberAfStatus emberAfCustomEndDeviceConfigurationClusterServerPreAttributeChangedCallback(int8u endpoint,
                                                                                          EmberAfAttributeId attributeId,
                                                                                          EmberAfAttributeType attributeType,
                                                                                          int8u size,
                                                                                          int8u *value)
{

  emberAfCorePrintln(" ===========in callback = 0x%02X ",attributeId);
  return 1;



}
void Sensor_interrupt_CB(uint8_t pin)
{
  emberAfCorePrintln("Sensor_interrupt_CB %d",pin);

  if(pin == 0)//Sensor Intr Pin is 0
  {
    if(GPIO_PinInGet(PORT_SENSOR_INT, PIN_SENSOR_INT))
    {
      emberAfCorePrintln("sensor_intr %d  HIGH",pin);
      set_proximity_level();
    }
  }
}

void init_GPIOs()
{
  GPIOINT_Init();
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(DIM_PORT, DIM_PIN, gpioModeWiredAndPullUpFilter, 1);


  GPIO_PinModeSet(PORT_SENSOR_INT, PIN_SENSOR_INT, gpioModeInputPull, 1);
  GPIO_ExtIntConfig(PORT_SENSOR_INT, PIN_SENSOR_INT, PIN_SENSOR_INT, true, true, true);
  GPIOINT_CallbackRegister(PIN_SENSOR_INT, Sensor_interrupt_CB);

}

void emberAfMainInitCallback(void)
{
  //emberEventControlSetActive(closeNetworkEventControl);

//Saving the data from flash memory of the Boot Count Cluster
  halCommonGetToken(&Boot_Count,TOKEN_BOOT_COUNT_1);
   Boot_Count++;
   int value[1]={Boot_Count};
   emberAfWriteManufacturerSpecificServerAttribute(Endpoint_1, ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                   0x000D, EMBER_AF_MANUFACTURER_CODE, (uint8_t*)value, ZCL_INT32U_ATTRIBUTE_TYPE);
   emberAfCorePrintln("====Boot_Count :  %d  ", Boot_Count);

  emberEventControlSetActive(Button_Isr_Poll_Control);
  init_GPIOs();

  interface_iic_init();
  MCP23017_Init();

  Dim_Init();
  Serial_Init();
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
//      emberAfCorePrint("%X", btn6Buff[i]);
  }
  emberAfCorePrintln("");

 // read_NVM_data();

}
