
#include <App_Serial.h>
#include <stdbool.h>
#include <em_usart.h>
#include "em_timer.h"
#include <stdint.h>
#include "em_cmu.h"
#include "zaf_event_distributor_soc.h"
#include <board_config.h>
#include <AppTimer.h>
#include <SwTimer.h>
#include "DebugPrintConfig.h"
#define DEBUGPRINT
#include "DebugPrint.h"

#include <string.h>
volatile uint8_t CheckSum = 0;
volatile uint16_t Rx_Cmd_Len = 0;
volatile uint8_t Current_State = 0;
volatile uint8_t Previous_State = 0;
volatile uint8_t Temp_Current_State = 0;
volatile uint16_t Current_Level = 0;
volatile uint16_t Previous_Level = 0;
volatile uint16_t Temp_Current_Level = 0;
volatile SERIAL_RX_STATE Rx_State = STATE_HEADER_1;
static uint8_t Rx_Buff[RX_BUFF_LEN] = {0};
static uint8_t Rx_Buff_Len = 0;
 uint16_t count = 0;
uint16_t count1 = 0;
uint16_t count2 = 0;
uint8_t eight=0;

uint8_t rotaryLevel = 0;
bool receive = true;

#define RX_BUFF_LEN1 256

static uint8_t  Rx_Buff1[RX_BUFF_LEN1] = {0};
static uint16_t Rx_Buff_Len1 = 0;


void sendFrame(uint8_t *frame);

uint8_t updateKnobLevel = 0;

SSwTimer learn_reset_timer;
void learn_reset_timer_CB();

//--------------

//---------
/*void learn_reset_timer_CB()
{
	count = 0;
  count1 = 0;
  count2 = 0;

}*/
uint8_t Current_Button_rx = 0;
uint8_t Current_Count_rx = 0;
void SceneButtonPress_rx(uint8_t button_rx)
{
   static uint8_t Privious_Button_rx = 0;
  if(Privious_Button_rx == button_rx)
  {
    Current_Count_rx++;
  //  DPRINTF("*** **cCurrent_Count_rx %d pressed  .\n", Current_Count_rx);

  }
  else
  {

    Privious_Button_rx = button_rx;
    Current_Count_rx = 1;
  }
  Current_Button_rx = button_rx;
  TimerStart(&learn_reset_timer,1000);


}
void learn_reset_timer_CB()
{
   if( (Current_Button_rx >= 1 && Current_Button_rx <= 3) && Current_Count_rx >= 4)
  {
       DPRINTF("****************Button %d pressed  .\n", Current_Button_rx);
       button_pressed_num=Current_Button_rx;
       Current_Button_rx = 0;
          Current_Count_rx = 0;
  }
   Current_Button_rx = 0;
     Current_Count_rx = 0;
 }
//uint8_t txFrame1on[] = {
//    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x11, 0x84,
//    0x01, 0x85, 0x91, 0x86, 0x00, 0x87, 0x81, 0x48, 0x5E
//};
//
uint8_t txFrame1off[] = {
    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x11, 0x84,
    0x01, 0x85, 0x91, 0x86, 0x00, 0x87, 0x80, 0x48, 0x5F
};
//
uint8_t txFrame39[] = {
    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
    0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x64,
    0x49, 0x3B
};
//
//uint8_t txFrame10[] = {
//    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
//    0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x1A,
//    0x49, 0x45
//};
//
//uint8_t txFrame20[] = {
//    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
//    0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x33,
//    0x49, 0x6C
//};
//
//uint8_t txFrame50[] = {
//    0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
//    0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x80,
//    0x49, 0xDF
//};
volatile uint16_t txIndex = 0; // Index to track the current position in txFrame
volatile uint16_t txFrameLength = sizeof(txFrame39) / sizeof(txFrame39[0]); // Length of txFrame
uint8_t *currentTxFrame; // Pointer to the current frame to transmit

uint8_t hexValue=0;
void Set_Value(uint8_t ep, uint8_t val)
{
	UNUSED(val);
	if(ep == 1)
	{
      SceneButtonPress_rx(1);

	    if(val==170)
	      {
	        zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_4);

	      }
	    zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_1);
//		count++;
//		TimerStart(&learn_reset_timer,500);
    DPRINTF("setvalue(%d, %d%%)\n", ep, val); // Replace with actual function implementation
       rotaryLevel = val;
         hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);


	}
	if(ep == 2)
	{
      SceneButtonPress_rx(2);

      if(val==170)
        {
          zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_5);

        }
  //    zaf_event_distributor_enqueue_app_event(EVENT_SYSTEM_LEARNMODE_TOGGLE);
	    DPRINTF("setvalue(%d, %d%%)\n", ep, val); // Replace with actual function implementation

	    zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_2);
/*	    count1++;
	      TimerStart(&learn_reset_timer,500);*/
	       rotaryLevel = val;
	         hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);
	   //  DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
	}
	if(ep == 3)
	{
      SceneButtonPress_rx(3);

      if(val==170)
        {
          zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_6);

        }
	    DPRINTF("setvalue(%d, %d%%)\n", ep, val); // Replace with actual function implementation

	    zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_3);
/*      count2++;
        TimerStart(&learn_reset_timer,500);*/
        rotaryLevel = val;
          hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);
   //   DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
	}

/*  if(count == 2)
  {
      button_pressed_num=2;

      DPRINTF("\n SEC set and button pressed is %d \n",button_pressed_num);
      count=0;
      //zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SW1_TRIPLE_PRESS);
  }
  if(count1 == 2)
  {
       button_pressed_num=3;

      DPRINT("\n sec set \n");
      count1=0;

  }
      if(count2 == 2)
          {
           button_pressed_num=1;
           DPRINTF("\n third set and button pressed is %d \n",button_pressed_num);
           count2=0;

          }*/

}


//Receive data from touch IC
void USART1_RX_IRQHandler(void)
{
 // DPRINTF(" hi in rx handler \n ");

  uint32_t flags;
  flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);

  uint8_t data;

  //start a timer(HW timer)
  TIMER_CounterSet(TIMER1,1);
  TIMER_Enable(TIMER1,true);

  data = USART_Rx(USART1);
  Rx_Buff[Rx_Buff_Len] = data;
    Rx_Buff_Len++;
//  DPRINTF("%X ",data);

}

/**************************************************************************//**
 * @brief USART0 TX interrupt service routine
 *****************************************************************************/

void USART1_TX_IRQHandler(void)
{
//  DPRINTF("\n IN TX HANDLER ...\n");
  uint32_t flags;
  flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);

  if (flags & USART_IF_TXC)
  {
    //
  }
}
uint8_t databyte;
void TIMER1_IRQHandler(void)
{
    // Acknowledge the interrupt
    uint32_t flags = TIMER_IntGet(TIMER1);
    TIMER_IntClear(TIMER1, flags);
    TIMER_Enable(TIMER1, false);

    // Print the entire received frame in hexadecimal format
 //   DPRINTF("Received Frame: ");
    for (int i = 0; i < Rx_Buff_Len; i++) {
        DPRINTF("%02X ", Rx_Buff[i]);
    }
    DPRINTF("\n");

    // Determine the endpoint (ep) from Rx_Buff[8] using switch-case
    uint8_t ep = 0;
    switch (Rx_Buff[8])
    {
        case 0x33: ep = 1; break;
        case 0x34: ep = 2; break;
        case 0x35: ep = 3; break;
        default:
            DPRINTF("Invalid EP in Rx_Buff[8]: %02X\n", Rx_Buff[8]);
            break;
    }
    if(ep == 0)
    {
        // Invalid endpoint – no further processing
        goto cleanup;
    }

    // Process frame based on the frame type in Rx_Buff[7]
    switch (Rx_Buff[7])
    {
        case 0x0D:  // Dimming frame
        {
            // Verify that data length (Rx_Buff[16]) is expected (0x04)
            if(Rx_Buff[16] != 0x04) {
                DPRINTF("Unexpected data length for dimming frame: %02X\n", Rx_Buff[16]);
                break;
            }
            // Now check Rx_Buff[13] to distinguish normal dimming vs. color temperature
            switch(Rx_Buff[13])
            {
              case 0x01: // Curtain stop command
              {
                  if (Rx_Buff[20] == 0X70)
                  {
                      DPRINTF("CURTAIN STOP: EP=%d\n", ep);
                      Set_Value(ep, 170); // Optional: You can define a special value to represent stop

                  }


              }
              break;

              case 0x02: // Curtain control (direct decimal percentage)
              {
                  uint8_t databyte = Rx_Buff[20];       // e.g., 0x64 will be 100
                  int final_percent = (databyte > 100) ? 100 : databyte;  // Cap at 100

                  DPRINTF("CURTAIN : EP=%d, Calculated Percentage=%d%%\n", ep, final_percent);
                  Set_Value(ep, final_percent);  // Use the actual decimal value
              }
              break;

                case 0x03: // Normal dimming
                {
                    // Process the dimming value from Rx_Buff[20]
                    uint8_t databyte = Rx_Buff[20];
                    uint8_t units = databyte % 10;         // Extract unit's digit
                    uint8_t remaining = databyte - units;    // Extract remaining digits

                    int base_percent = -1;
                    // Determine base percentage based on unit's digit.
                    switch(units)
                    {
                        case 0: base_percent = 0; break;
                        case 4: base_percent = 26; break;
                        case 8: base_percent = 52; break;
                        case 2: base_percent = 77; break;
                        default:
                            DPRINTF("Invalid unit digit for normal dimming: %d\n", units);
                            break;
                    }
                    if(base_percent == -1)
                        break;

                    int increment = remaining / 10;  // +1% per full 10 in remaining digits
                    int final_percent = base_percent + increment;
                    if(final_percent > 100)
                        final_percent = 100;

                    DPRINTF("Normal Dimming: EP=%d, Calculated Percentage=%d%%\n", ep, final_percent);
                    Set_Value(ep, final_percent);
                }
                break;

                case 0x04: // Color temperature
                {
                    // Process the value from Rx_Buff[20] for color temperature.
                    // For demonstration, we'll use the same calculation logic, but call Set_ColorTemp.
                    uint8_t databyte = Rx_Buff[20];
                    uint8_t units = databyte % 10;         // Extract unit's digit
                    uint8_t remaining = databyte - units;    // Extract remaining digits

                    int base_value = -1;
                    // Use the same base logic as normal dimming (adjust if needed)
                    switch(units)
                    {
                        case 0: base_value = 0; break;
                        case 4: base_value = 26; break;
                        case 8: base_value = 52; break;
                        case 2: base_value = 77; break;
                        default:
                            DPRINTF("Invalid unit digit for color temperature: %d\n", units);
                            break;
                    }
                    if(base_value == -1)
                        break;

                    int increment = remaining / 10;
                    int final_value = base_value + increment;
                    if(final_value > 100)
                        final_value = 100;

                    DPRINTF("Color Temperature: EP=%d, Calculated Value=%d\n", ep, final_value);
                   // Set_ColorTemp(ep, final_value);
                }
                break;

                default:
                    DPRINTF("Invalid payload type in dimming frame (Rx_Buff[13]): %02X\n", Rx_Buff[13]);
                    break;
            }
        }
        break;

        case 0x0A:  // Off frame
        {
            // Verify that data length (Rx_Buff[16]) is expected (0x01)
            if(Rx_Buff[16] != 0x01) {
                DPRINTF("Unexpected data length for off frame: %02X\n", Rx_Buff[16]);

                break;
            }
            if(Rx_Buff[17] == 0x00) {
                DPRINTF("Off Frame: EP=%d, Off Value=%02X\n", ep, 0);
                Set_Value(ep, 0);
                   break;
               }
            // Use Rx_Buff[17] as the off value
/*            uint8_t off_val = Rx_Buff[17];
            DPRINTF("Off Frame: EP=%d, Off Value=%02X\n", ep, off_val);
            Set_Value(ep, off_val);*/

        }
        break;
        case 0x0E:
          {
                       if (Rx_Buff[21] == 0X65)
                            {
                                DPRINTF("CURTAIN OFF: EP=%d\n", ep);
                                Set_Value(ep, 0); // Optional: You can define a special value to represent stop

                            }
          }
          break;

        default:
           // DPRINTF("Invalid frame type in Rx_Buff[7]: %02X\n", Rx_Buff[7]);
            break;
    }

cleanup:
    // Reset the buffer length for the next frame
    Rx_Buff_Len = 0;
}
/*void Set_ColorTemp(uint8_t ep, int val)
{
    // Print the color temperature settings.
    DPRINTF("Color Temp: EP=%d, Value=%d\n", ep, val);
}*/




void Init_Hw_Timer()
{

  // Enable TIMER1
  CMU_ClockEnable(cmuClock_TIMER1, true);

  //GPIO_PinModeSet(gpioPortA, 3, gpioModePushPull, 1);		//Rx
  // Set the PWM frequency
  TIMER_TopSet(TIMER1, 0xffff);

  // Create the timer count control object initializer
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

//  timerCCInit.cmoa            = timerOutputActionSet;//timerOutputActionToggle;
  timerCCInit.mode      	  = timerCCModeCompare;

  // Configure CC channel 0
  TIMER_InitCC(TIMER1, 0, &timerCCInit);

   // Initialize and start timer with defined prescale
   TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
   timerInit.prescale = timerPrescale2;
   TIMER_Init(TIMER1, &timerInit);

   TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
   NVIC_EnableIRQ(TIMER1_IRQn);
}

void Serail_Init()
{
//  DPRINTF(" hi in serial init \n");

	CMU_ClockEnable(cmuClock_USART1, true);

	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
	init.baudrate = 115200;

	// set pin modes for USART TX and RX pins
	GPIO_PinModeSet(SERIAL_RX_PORT, SERIAL_RX_PIN, gpioModeInput, 0);		//Rx
	GPIO_PinModeSet(SERIAL_TX_PORT, SERIAL_TX_PIN, gpioModePushPull, 1);	//Tx

	// Initialize USART asynchronous mode and route pins
	USART_InitAsync(USART1, &init);
	USART1->ROUTELOC0 = USART_ROUTELOC0_RXLOC_LOC11 | USART_ROUTELOC0_TXLOC_LOC11;
	USART1->ROUTEPEN |= USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN; ;

	//Initializae USART Interrupts
	USART_IntEnable(USART1, USART_IEN_RXDATAV);
	USART_IntEnable(USART1, USART_IEN_TXC);

	//Enabling USART Interrupts
	NVIC_EnableIRQ(USART1_RX_IRQn);
	NVIC_EnableIRQ(USART1_TX_IRQn);

	Init_Hw_Timer();
	bool status=AppTimerRegister(&learn_reset_timer, true, learn_reset_timer_CB);
	DPRINTF("\n Serial init,status is %d \n",status);
}




void USART2_TX_IRQHandler(void)
{
 // DPRINTF("\n  Transmission completed");

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);
    if (flags & USART_IF_TXC)
    {
      //  DPRINTF("\nTransmission completed\n");
    }
  //  GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);
   // GPIO_PinOutClear(READ_PORT, READ_PIN);
}




#define FRAME_START_30 0x30  // Starting byte for frame type 0x30
#define FRAME_START_B0 0xb0  // Starting byte for frame type 0xB0
#define FRAME_SIZE 75        // Maximum expected frame size

bool frame_started_30 = false; // Flag for frame type 0x30
bool frame_started_b0 = false; // Flag for frame type 0xB0
bool frame_started_1 = false;
#define FRAME_START_1 0xbc
void USART2_RX_IRQHandler(void)
{
 // DPRINTF("\reception  completed");

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);

    uint8_t data1;

    // Start a timer (HW timer)
    TIMER_CounterSet(TIMER0, 1);
    TIMER_Enable(TIMER0, true);

    // Read received data as char
    data1 = USART_Rx(USART2);

  // Check for frame start (0x30 or 0xB0)
  if (data1 == FRAME_START_30) {
    //  DPRINTF(" 0000000000000 ****\n");
      if (frame_started_30) {
          DPRINTF("\n"); // Print newline before new frame starts
          Rx_Buff_Len1 = 0; // Reset the buffer length
      }
      frame_started_30 = true;  // Mark that a frame of type 0x30 has started
      frame_started_b0 = false; // Reset the other flag
      frame_started_1=false;
  } else if (data1 == FRAME_START_B0) {
     // DPRINTF(" 11111111111 ****\n");
      if (frame_started_b0) {
          DPRINTF("\n"); // Print newline before new frame starts
          Rx_Buff_Len1 = 0; // Reset the buffer length
      }
      frame_started_b0 = true;  // Mark that a frame of type 0xB0 has started
      frame_started_30 = false; // Reset the other flag
      frame_started_1=false;

  }
  else if (data1 == FRAME_START_1) {
   //   DPRINTF(" 2222222222 ****\n");
           if (frame_started_1) {
               DPRINTF("\n"); // Print newline before new frame starts
               Rx_Buff_Len1 = 0; // Reset the buffer length
           }
           frame_started_1=true;
           frame_started_b0 = false;  // Mark that a frame of type 0xB0 has started
           frame_started_30 = false; // Reset the other flag
       }
    // Add the received character to the buffer
    Rx_Buff1[Rx_Buff_Len1] = data1;
    Rx_Buff_Len1++;
//      DPRINTF(" receiving ****\n");
   // eight= Rx_Buff1[8];

}
void TIMER0_IRQHandler(void)
{
 //  DPRINTF(" entered  \n ");

    uint32_t flags = TIMER_IntGet(TIMER0);
    TIMER_IntClear(TIMER0, flags);

    TIMER_Enable(TIMER0, false);
   // DPRINTF("\n, Hex Value: 0x%02X \n", hexValue);
  //  DPRINTF("Frame: ");
   // if(button_pressed_num==0)
   //   {
// DPRINTF("button not pressed \n ");
//
//   for (int i = 0; i < 11; i++)
 //  {
 // DPRINTF("%.2x ",Rx_Buff1[i]);

//
//     if(i==8 && hexValue!= Rx_Buff1[8])
//       {
//
//      extractDestinationValues(Rx_Buff1[3], Rx_Buff1[4],Rx_Buff1[8]);
      //     }
////
  // }
   //   }
//    else
    //  {
        if (Rx_Buff_Len1 > 0) {

              // Process frame type 0x30
              if (frame_started_30 && Rx_Buff1[0] == FRAME_START_30) {

                  if (Rx_Buff1[9] == 0x10 && Rx_Buff1[10] == 0x00) {
                      uint8_t length = Rx_Buff1[12]; // Extract length
                      DPRINTF("Frame 0x30 Group Address Length: 0x%.2X\n", length);
                      extract_and_print_group_addresses(Rx_Buff1, length, button_pressed_num);
                      DPRINTF("\n");
                  }
              }

              // Process frame type 0xB0
              if (frame_started_b0 && Rx_Buff1[0] == FRAME_START_B0) {

                  if (Rx_Buff1[8] == 0x10 && Rx_Buff1[9] == 0x00) {
                      uint8_t length = Rx_Buff1[11]; // Extract length
                      DPRINTF("Frame 0xB0 Group Address Length: 0x%.2X\n", length);

                      extract_and_print_group_addresses_1(Rx_Buff1, length, button_pressed_num);
                      DPRINTF("\n");
                  }
              }

              if(frame_started_1 && Rx_Buff1[0] == FRAME_START_1)
                {
                  //  DPRINTF("Frame: ");
                   for (int i = 0; i < 11; i++)
                  {
                   DPRINTF("%.2x ",Rx_Buff1[i]);


                    if(i==8 && hexValue!= Rx_Buff1[8])
                      {

                    // extractDestinationValues(Rx_Buff1[3], Rx_Buff1[4],Rx_Buff1[8]);
                          }

                  }
                   DPRINTF("\n");

                }

        //  }

          // Reset buffer state for the next frame


      }

    frame_started_30 = false; // Reset the flag for 0x30
    frame_started_b0 = false; // Reset the flag for 0xB0
    frame_started_1 = false;  // Reset the frame started flag

    Rx_Buff_Len1 = 0;
}


void Init_Hw_Timer2()
{

  CMU_ClockEnable(cmuClock_TIMER0, true);

  //GPIO_PinModeSet(gpioPortA, 3, gpioModePushPull, 1);   //Rx
   TIMER_TopSet(TIMER0, 0xffff);

  // Create the timer count control object initializer
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

   timerCCInit.mode          = timerCCModeCompare;

  // Configure CC channel 0
  TIMER_InitCC(TIMER0, 0, &timerCCInit);

   // Initialize and start timer with defined prescale
   TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
   timerInit.prescale = timerPrescale2;
   TIMER_Init(TIMER0, &timerInit);

   TIMER_IntEnable(TIMER0, TIMER_IEN_CC0);
   //NVIC_EnableIRQ(TIMER1_IRQn);
   NVIC_EnableIRQ(TIMER0_IRQn);

}

void Serail_Init1()
{
 //DPRINTF("IN USART2 \n");
  CMU_ClockEnable(cmuClock_USART2, true);

  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  init.baudrate = 19200;
  init.parity = usartEvenParity; // Set even parity

  // set pin modes for USART TX and RX pins
  GPIO_PinModeSet(SERIAL_RX_PORT1, SERIAL_RX_PIN1, gpioModeInput, 0);   //Rx
  GPIO_PinModeSet(SERIAL_TX_PORT1, SERIAL_TX_PIN1, gpioModePushPull, 1);  //Tx
 // GPIO_PinModeSet(READ_PORT, READ_PIN, gpioModePushPull, 0);  // Control GPIO
  //GPIO_PinModeSet(WRITE_PORT, WRITE_PIN, gpioModePushPull, 0); // Control GPIO

//  GPIO_PinOutClear(READ_PORT, READ_PIN);
  //GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);
   USART_InitAsync(USART2, &init);
  USART2->ROUTELOC0 = USART_ROUTELOC0_RXLOC_LOC16 | USART_ROUTELOC0_TXLOC_LOC16;
  USART2->ROUTEPEN |= USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  //Initializae USART Interrupts
  USART_IntEnable(USART2, USART_IEN_RXDATAV);
  USART_IntEnable(USART2, USART_IEN_TXC);

  //Enabling USART Interrupts
  NVIC_EnableIRQ(USART2_RX_IRQn);
  NVIC_EnableIRQ(USART2_TX_IRQn);

  Init_Hw_Timer2();
  AppTimerRegister(&learn_reset_timer, true, learn_reset_timer_CB);
  DPRINTF("\n Serial 2 init \n");
}


void sendFrame(uint8_t *frame)
{
//  DPRINTF("\nTx start");
//  GPIO_PinOutSet(WRITE_PORT,WRITE_PIN);
  //GPIO_PinOutSet(READ_PORT,READ_PIN);
  currentTxFrame = frame; // Set current frame to transmit

 // DPRINTF("TX Frame -> ");
  //int length = strlen(frame);
  for (int i = 0; i < txFrameLength; i++)
  {
      DPRINTF("0x%02X ", currentTxFrame[i]);
    USART_Tx(USART2,currentTxFrame[i]);
  }
 //DPRINTF("\nTx Done");
  DPRINTF("\n");

}


void updateKnobLevelhex(uint8_t updateKnobhex)
{
  DPRINTF("-----------------yes---  updated ------------: %X\n", updateKnobhex);

   static uint16_t seq = 0;
//
  uint8_t State_buf[17] = {0x55, 0xAA, 0x02, 0x00, 0x03, 0x04, 0x00, 0x08, 0x71, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x32, 0xB9};
   //  uint8_t knobHexValue = (uint8_t)((updateKnobLevel * 100) / 255);  // Scale 0-100% to 0x00-0xFF
    State_buf[15] = updateKnobhex;
  //button_event_happed = 1;

     DPRINTF("--r-level-: %u\n", updateKnobhex);
     uint16_t Sum = 0;
     State_buf[3] = ((seq & 0xFF00)>>0x08);
    State_buf[4] = (seq & 0x00FF);
     for(uint16_t i=0; i<16; i++)
     {
       Sum = Sum + State_buf[i];
     }
     State_buf[16] = (uint8_t)((Sum%256) & 0xFF);
       for(uint16_t i = 0; i<17; i++)
       {
        USART_Tx(USART1, State_buf[i]);
       }
       seq++;
}
