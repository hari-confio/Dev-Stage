
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
#include <CC_Configuration_Parameters.h>
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
bool addr_config_mode = false;
#define RX_BUFF_LEN1 256

static uint8_t  Rx_Buff1[RX_BUFF_LEN1] = {0};
static uint16_t Rx_Buff_Len1 = 0;


void sendFrame(uint8_t *frame);

uint8_t updateKnobLevel = 0;

SSwTimer learn_reset_timer;

void learn_reset_timer_CB();
void process_rs485_rx_data();
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

// Function to calculate Modbus CRC-16
uint16_t modbus_crc16(uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
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

uint8_t cmd_to_send[8];

void modbus_send_twice_CB(void)
{
  DPRINTF("\n==== MODBUS CH CMD1=====\n");
  sendFrame(cmd_to_send);
}

//04 03 00 10 13 01 28 AF -B1
void construct_frame(uint8_t ep, uint8_t val)
{

  //uint8_t cmd[8]={config_nvm_addr, 0x03, 0x00, 0x10, ep, val, 0x00, 0x00}; //Not 0x02 it's 0x10 for atlas defining
  uint8_t cmd[8]={config_nvm_addr, 0x03, 0x00, 0x10, ep, val, 0x00, 0x00}; //Not 0x02 it's 0x10 for atlas defining

  uint16_t crc = modbus_crc16(cmd, 6);
  cmd[6] = crc & 0xFF;
  cmd[7] = (crc >> 8) & 0xFF;
  sendFrame(cmd);
  for(int i=0;i<8;i++)
    {
      cmd_to_send[i] = cmd[i];
    }
  DPRINTF("\n==== MODBUS CH CMD=====\n");
  TimerStart(&modbus_send_twice, 100);

}

void knob_in_addr_mode_CB(void)
{
  //if(!addr_config_mode) construct_frame(active_ep, hexValue);
  DPRINTF("\n==== knob_in_addr_mode_CB=====\n");
  addr_config_mode = false;
}


uint8_t hexValue=0;
uint8_t active_ep=0;
void send_one_time_cmd_CB(void)
{
  DPRINTF("\n==== send_one_time_cmd_CB=====\n");
  if(!addr_config_mode) construct_frame(active_ep, hexValue);

}

void Set_Value(uint8_t ep, uint8_t val)
{
  rotaryLevel = val;
  active_ep = ep;
  hexValue = (uint8_t)(2.55 * rotaryLevel);//(uint8_t)((rotaryLevel / 100.0) * 255);
	if(ep == 1)
	{
     DPRINTF("====Rotary: EP = 1, Percentage = %d - %02X\n", val, hexValue);
	}
	else if(ep == 2)
  {
     DPRINTF("====Rotary: EP = 2, Percentage = %d - %02X\n", val, hexValue);
  }
	else if(ep == 3)
  {
     DPRINTF("====Rotary: EP = 3, Percentage = %d - %02X\n", val, hexValue);
  }

	TimerStart(&send_one_time_cmd, 400);

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
  //DPRINTF("\n IN TX HANDLER ...\n");
  uint32_t flags;
  flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);

  if (flags & USART_IF_TXC)
  {
    //
  }
}
uint8_t databyte;
bool enable_config_addr = false;
void TIMER1_IRQHandler(void)
{
    // Acknowledge the interrupt
    uint32_t flags = TIMER_IntGet(TIMER1);
    TIMER_IntClear(TIMER1, flags);
    TIMER_Enable(TIMER1, false);

    // Print the entire received frame in hexadecimal format
 //   DPRINTF("Received Frame: ");
   // for (int i = 0; i < Rx_Buff_Len; i++) {
        //DPRINTF("%02X ", Rx_Buff[i]);
   // }
   // DPRINTF("\n");

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
                      //Set_Value(ep, 170); // Optional: You can define a special value to represent stop

                  }


              }
              break;

              case 0x02: // Curtain control (direct decimal percentage)
              {
                  uint8_t databyte = Rx_Buff[20];       // e.g., 0x64 will be 100
                  int final_percent = (databyte > 100) ? 100 : databyte;  // Cap at 100

                  //DPRINTF("CURTAIN : EP=%d, Calculated Percentage=%d%%\n", ep, final_percent);
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

                    //DPRINTF("Normal Dimming: EP=%d, Calculated Percentage=%d%%\n", ep, final_percent);

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
                process_rs485_rx_data();
                enable_config_addr = true;
                time_config_addr_CB();
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
                                process_rs485_rx_data();
                                enable_config_addr = true;
                                time_config_addr_CB();
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

void Serail_Init() // ATLAS
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
	//bool status=AppTimerRegister(&learn_reset_timer, true, learn_reset_timer_CB);
	//DPRINTF("\n Serial init,status is %d \n",status);
}




void USART2_TX_IRQHandler(void)
{
  //DPRINTF("\n  Transmission completed");

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);
    if (flags & USART_IF_TXC)
    {
      //  DPRINTF("\nTransmission completed\n");
    }
    GPIO_PinOutClear(RS485_ENABLE_PORT, RS485_ENABLE_PIN);
    //GPIO_PinOutClear(READ_PORT, READ_PIN);

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
  //DPRINTF("\reception  completed");

    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);

    uint8_t data1;

    // Start a timer (HW timer)
    TIMER_CounterSet(TIMER0, 1);
    TIMER_Enable(TIMER0, true);

    // Read received data as char
    data1 = USART_Rx(USART2);

    Rx_Buff1[Rx_Buff_Len1] = data1;
    Rx_Buff_Len1++;


}
void TIMER0_IRQHandler(void)
{

    uint32_t flags = TIMER_IntGet(TIMER0);
    TIMER_IntClear(TIMER0, flags);

    TIMER_Enable(TIMER0, false);


   for (int i = 0; i < Rx_Buff_Len1; i++)
   {
       DPRINTF("%02X ",Rx_Buff1[i]);

   }
   DPRINTF("\n");

   process_rs485_rx_data();
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
  init.baudrate = 9600;


  // set pin modes for USART TX and RX pins
  GPIO_PinModeSet(SERIAL_RX_PORT1, SERIAL_RX_PIN1, gpioModeInputPull, 1);   //Rx
  GPIO_PinModeSet(SERIAL_TX_PORT1, SERIAL_TX_PIN1, gpioModePushPull, 1);  //Tx
  GPIO_PinModeSet(RS485_ENABLE_PORT, RS485_ENABLE_PIN, gpioModePushPull, 1);  // Control GPIO
  //GPIO_PinModeSet(WRITE_PORT, WRITE_PIN, gpioModePushPull, 0); // Control GPIO

  GPIO_PinOutClear(RS485_ENABLE_PORT, RS485_ENABLE_PIN);
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
  //AppTimerRegister(&learn_reset_timer, true, learn_reset_timer_CB);
  DPRINTF("\n Serial 2 init \n");
}


void sendFrame(uint8_t *frame)
{
//  DPRINTF("\nTx start");
  GPIO_PinOutSet(RS485_ENABLE_PORT,RS485_ENABLE_PIN);
  //GPIO_PinOutSet(READ_PORT,READ_PIN);

  for (int i = 0; i < MODBUS_FRAME_SIZE; i++)
  {
    DPRINTF("%02X ", frame[i]);
    USART_Tx(USART2, frame[i]);
  }
 //DPRINTF("\nTx Done");
  DPRINTF("\n");

}

void time_config_addr_CB()
{
  if(enable_config_addr && Rx_Buff1[0] == 0xFF && Rx_Buff1[1] == 0x06 && Rx_Buff1[2] == 0x10 && Rx_Buff1[3] == 0x50)
  {
      DPRINT("\n Address Configuration Done\n");
     // TimerStart(&time_config_addr,10);
      ApplicationData1.config_addr = Rx_Buff1[5];
      writeAppData(&ApplicationData1);
      uint8_t configuration_save_ack[8] = {0xFF, 0x06, 0x10, 0x50, 0xFF, 0xFF, 0x99, 0x75};
      for(uint8_t i=0;i<5;i++){
          sendFrame(configuration_save_ack);//Ack 5 times
      }
      DPRINT("\n Addr Save Ack Sent\n");
      addr_config_mode = true;
      config_nvm_addr = Rx_Buff1[5];
      TimerStop(&time_config_addr);
      TimerStart(&knob_in_addr_mode, 100);
  }
  else
  {
      DPRINT("\n Address Configuration Not Done/Enabled\n");
  }
  enable_config_addr = false;
  memset(Rx_Buff1, 0, Rx_Buff_Len1);
}

void process_rs485_rx_data()
{
  ////FD 03 10 0B 00 01 E5 34 00
//  if(Rx_Buff1[0] == 0xFD && Rx_Buff1[1] == 0x03 && Rx_Buff1[2] == 0x10 && Rx_Buff1[3] == 0x0B && Rx_Buff1[6] == 0xE5 && Rx_Buff1[7] == 0x34)
//    {
//
//      got_poll_cmd = true;
//
//    }///
  //ATLAS Address Configuration Mode
  //if(enable_config_addr){
   // DPRINT("\n Ready for Address Configuration \n");
 if(Rx_Buff1[0] == 0xFF && Rx_Buff1[1] == 0x06 && Rx_Buff1[2] == 0x10 && Rx_Buff1[3] == 0x50 && Rx_Buff1[4] == 0x00)
    {
      DPRINT("\n Ready for Address Configuration \n");
      addr_config_mode = true;
      TimerStart(&time_config_addr,60000);

    }
 else if(Rx_Buff1[0] == 0xFF && Rx_Buff1[1] == 0x06 && Rx_Buff1[6] == 0x99 && Rx_Buff1[7] == 0x75)
    {
      //DPRINT("\n Ready for Address Configuration \n");
      TimerStop(&time_config_addr);
      addr_config_mode = false;
      memset(Rx_Buff1, 0, Rx_Buff_Len1);
      enable_config_addr = false;
    }

}
