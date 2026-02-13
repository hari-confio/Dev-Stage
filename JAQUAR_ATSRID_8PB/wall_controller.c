
#include <Wall_App_Serial.h>
#include <stdbool.h>
#include <em_usart.h>
#include "em_timer.h"
#include <stdint.h>
#include <string.h>
#include "em_cmu.h"

#include "board_init.h"
#include <AppTimer.h>
#include <SwTimer.h>
#include "DebugPrintConfig.h"

#define DEBUGPRINT
#include "DebugPrint.h"
#include "MCP23017_I2C.h"

#define RX_BUFF_LEN 256
static uint8_t Rx_Buff[RX_BUFF_LEN] = {0};
static uint16_t Rx_Buff_Len = 0;
bool addr_config_mode = false, enable_addr_mode = false;
uint8_t defined_keypadID = 0;
SApplicationData ApplicationData;
SSwTimer led_inc_toggle;
SSwTimer make_rs485_low;
bool got_poll_cmd = false;
uint8_t make_leds_back_buff[8] = {1, 1, 1, 1, 1, 1, 1, 1};
void process_rs485_rx_data();
 void USART1_TX_IRQHandler(void)
 {
  // DPRINTF("\n  Transmission completed");

     uint32_t flags;
     flags = USART_IntGet(USART1);
     USART_IntClear(USART1, flags);
     if (flags & USART_IF_TXC)
     {
        // DPRINTF("\nTransmission completed");
     }
    // TimerStart(&make_rs485_low, 10);
     GPIO_PinOutClear(RS485_ENABLE_PORT, RS485_ENABLE_PIN);
 }

 void make_rs485_low_CB()
 {
  // GPIO_PinOutClear(RS485_ENABLE_PORT, RS485_ENABLE_PIN);
 }
 // === CRC16 MODBUS ===
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

void sendFrame(uint8_t *frame)
{
//  DPRINTF("\nTx start");
  GPIO_PinOutSet(RS485_ENABLE_PORT,RS485_ENABLE_PIN);
  for (int i = 0; i < FRAME_SIZE; i++)
  {
   DPRINTF("%02X ", frame[i]);
    USART_Tx(USART1,frame[i]);
  }
 DPRINTF("\nTx Done");

}

void Init_Hw_Timer()
{
    // Enable TIMER1
    CMU_ClockEnable(cmuClock_TIMER1, true);

    // Set the PWM frequency
    TIMER_TopSet(TIMER1, 0xffff);

    // Create the timer count control object initializer
    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
    timerCCInit.mode = timerCCModeCompare;

    // Configure CC channel 0
    TIMER_InitCC(TIMER1, 0, &timerCCInit);

    // Initialize and start timer with defined prescale
    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    timerInit.prescale = timerPrescale2;
    TIMER_Init(TIMER1, &timerInit);

    TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
    NVIC_EnableIRQ(TIMER1_IRQn);
    DPRINT("hw_timer\n");

}

void TIMER1_IRQHandler(void) {
    uint32_t flags = TIMER_IntGet(TIMER1);
    TIMER_IntClear(TIMER1, flags);
    TIMER_Enable(TIMER1, false);
//    for(int i=0; i<FRAME_SIZE; i++){
//        DPRINTF("%02x ", Rx_Buff[i]);
//    }
//    DPRINTF("\n");
    process_rs485_rx_data();
    Rx_Buff_Len = 0;
}

void Serial_Init()
{
  DPRINT("Serial_Init\n");
  CMU_ClockEnable(cmuClock_USART1, true);

  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  init.baudrate = 9600;

  // Set pin modes for USART TX and RX pins
  GPIO_PinModeSet(SERIAL_RX_PORT, SERIAL_RX_PIN, gpioModeInput, 0);   // Rx
  GPIO_PinModeSet(SERIAL_TX_PORT, SERIAL_TX_PIN, gpioModePushPull, 1); // Tx
  GPIO_PinModeSet(RS485_ENABLE_PORT, RS485_ENABLE_PIN, gpioModePushPull, 0);

  GPIO_PinOutClear(RS485_ENABLE_PORT,RS485_ENABLE_PIN);
  // Initialize USART asynchronous mode and route pins
  USART_InitAsync(USART1, &init);
  USART1->ROUTELOC0 = USART_ROUTELOC0_RXLOC_LOC20 | USART_ROUTELOC0_TXLOC_LOC20;
  USART1->ROUTEPEN |= USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  // Initialize USART Interrupts
  USART_IntEnable(USART1, USART_IEN_RXDATAV);
  USART_IntEnable(USART1, USART_IEN_TXC);

  // Enable USART Interrupts
  NVIC_EnableIRQ(USART1_RX_IRQn);
  NVIC_EnableIRQ(USART1_TX_IRQn);

  Init_Hw_Timer();

}
//===========

SSwTimer smooth_led_dim_timer;

uint8_t dim_target_value = 100;
uint8_t dim_current_value = 100;
uint32_t timeCount = 0;

uint8_t parameter_DIM_TIME = 23;
uint8_t parameter_MIN_LEVEL= 0;
uint8_t parameter_MAX_LEVEL= 100;

void smooth_led_dim_timer_CB()
{
 // DPRINTF("  smooth_led_dim_timer_CB\n");

  if(dim_target_value > dim_current_value)
  {
//      DPRINTF(" dim_target_value > dim_current_value \n");
    //  DPRINTF(" dim_target_value-- %d\n",dim_target_value);
    //  DPRINTF(" dim_current_value-- %d\n",dim_current_value);

    Set_Dim_levels(dim_current_value++);
    timeCount = 0;

  }
  else if(dim_target_value < dim_current_value)
  {
//      DPRINTF(" dim_current_value-- %d\n",dim_current_value);
//
//      DPRINTF(" dim_target_value < dim_current_value \n");
//      DPRINTF(" dim_target_value-- %d\n",dim_target_value);

    Set_Dim_levels(dim_current_value--);
    timeCount = 0;

  }
  else
  {
    //  DPRINTF("in timeCount == parameter_DIM_TIME*100 . \n");

    timeCount++;
    if(timeCount == parameter_DIM_TIME*100)//parameter_value_9*1000
    {
      dim_target_value = parameter_MIN_LEVEL;
    //  DPRINTF(" dim_target_value-- %d\n",dim_target_value);
 //     DPRINTF(" dim_current_value-- %d\n",dim_current_value);

    }

    if(dim_current_value == parameter_MIN_LEVEL)
    {
        //DPRINTF(" in else ... \n");
       // DPRINTF(" dim_target_value-- %d\n",dim_target_value);

      TimerStop(&smooth_led_dim_timer);
      Set_Dim_levels(dim_current_value);
     // DPRINTF(" dim_current_value-- %d\n",dim_current_value);


    }

    if(timeCount%100 == 0)
    {
     // DPRINTF("D-%d-%d \n",parameter_DIM_TIME,timeCount/100);
    //  DPRINTF("in timecount . \n");

    }
  }
}



void set_proximity_level()
{
 // DPRINTF("  set_proximity_level\n");

  TimerStart(&smooth_led_dim_timer,10);
  dim_target_value =  parameter_MAX_LEVEL;
  timeCount = 0;
}

uint32_t triac_compare_val1_1;

void Set_Dim_levels(uint8_t Dim)
{
 // DPRINTF("  Set_Dim_levels\n");

  uint32_t max = 0xBE6E;
  triac_compare_val1_1 = (max * (uint32_t)(Dim)) / 100;
  TIMER_CompareSet(TIMER0, 1, triac_compare_val1_1);
}


void Dim_Init()
{
  DPRINTF("DIM INIT intialisation completed \n");

  CMU_ClockEnable(cmuClock_TIMER0, false);
  TIMER_Enable(TIMER0, false);
  GPIO_PinModeSet(PORT_DIM, PIN_DIM, gpioModePushPull, OFF);

//  GPIO_PinModeSet(PORT_DIM, PIN_DIM, gpioModePushPull, ON);

  // Enable clock for TIMER0 module
  CMU_ClockEnable(cmuClock_TIMER0, true);

  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode = timerCCModePWM;

  TIMER_InitCC(TIMER0, 1, &timerCCInit);  //output triac 1

  //Set First compare value fot Triac 1
  uint32_t max = 0xBE6E;//800Hz
  triac_compare_val1_1 = (max * (uint32_t)(100)) / 100;
  TIMER_CompareSet(TIMER0, 1, triac_compare_val1_1);

  //Set Top value for Timer (should be less then 20mSec)
  max = 0xBE6E;//800Hz
  TIMER_TopSet(TIMER0, max);

  TIMER0->ROUTELOC0 = (TIMER0->ROUTELOC0 & (~_TIMER_ROUTELOC0_CC1LOC_MASK)) | TIMER_ROUTELOC0_CC1LOC_LOC29;//PF6
  TIMER0->ROUTEPEN  = TIMER_ROUTEPEN_CC1PEN;

  // Initialize and start timer
  TIMER_Init_TypeDef timerInit_TIM_LEC = TIMER_INIT_DEFAULT;
  timerInit_TIM_LEC.prescale = timerPrescale1;

  TIMER_Init(TIMER0, &timerInit_TIM_LEC);
  TIMER_Enable(TIMER0, true);

  AppTimerRegister(&smooth_led_dim_timer, true, smooth_led_dim_timer_CB);
}


// USART interrupt handler
void USART1_RX_IRQHandler(void) {
  uint32_t flags = USART_IntGet(USART1);
  USART_IntClear(USART1, flags);
  uint8_t data;

  // Start a timer (HW timer)
  TIMER_CounterSet(TIMER1, 1);
  TIMER_Enable(TIMER1, true);

  data = (uint8_t) USART_Rx(USART1);
 // DPRINTF("%02X ", data);
  if (Rx_Buff_Len < RX_BUFF_LEN) // Ensure no overflow
  {
      Rx_Buff[Rx_Buff_Len++] = data;
      //DPRINTF("%02X ", data);
  }
  else
  {
      DPRINTF("Rx_Buff overflow");
  }

}

bool make_on = false;
void led_inc_toggle_CB()
{
  set_proximity_level();
  //DPRINTF("----led_inc_toggle_CB----\n");
  //Blink LEDs to indicate Addr config mode
  if(make_on){
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
    make_on = false;
  }
  else{
    MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 0);

    MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
    MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
    make_on = true;
  }
  TimerStart(&led_inc_toggle, 100);
}
uint8_t cmd_to_send[8];

void modbus_send_twice_CB()
{
  DPRINTF("\n==== MODBUS CMD1=====\n");
  sendFrame(cmd_to_send);
}

bool led_came = false;
uint8_t btn_cmd[FRAME_SIZE];
void createFrame(uint8_t btn)
{
  set_proximity_level();
/*04 03 00 02 13 01 28 AF -B1
  04 03 00 02 14 02 6A 9E -B2
  02 03 00 02 09 04 E3 AA -B3
  02 03 00 02 0A 08 E3 5F -B4
  02 03 00 02 0B 10 E2 C5 -B5
  02 03 00 02 0C 20 E0 E1 -B6
  40 - B7, 80 - B8*/
  btn_cmd[0] = defined_keypadID;
  btn_cmd[1] = 0x03;
  btn_cmd[2] = 0x00;
  btn_cmd[3] = 0x02;
  btn_cmd[4] = 0x00;

  if(btn == 1)
    {
      btn_cmd[5] = 0x01;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 2)
    {
      btn_cmd[5] = 0x02;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 3)
    {
      btn_cmd[5] = 0x04;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 4)
    {
      btn_cmd[5] = 0x08;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 5)
    {
      btn_cmd[5] = 0x10;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 6)
    {
      btn_cmd[5] = 0x20;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 7)
    {
      btn_cmd[5] = 0x40;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
  else if(btn == 8)
    {
      btn_cmd[5] = 0x80;
      uint16_t crc = modbus_crc16(btn_cmd, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      btn_cmd[6] = crc1;
      btn_cmd[7] = crc2;
    }
 // while(!led_came) {
      //DPRINTF("READY\n");
  //for(int j=0;j<5;j++){
 // if(got_poll_cmd){
 // GPIO_PinOutSet(RS485_ENABLE_PORT,RS485_ENABLE_PIN);
      sendFrame(btn_cmd);
   //   for(int i=0;i<8;i++)
       // {
       ////   cmd_to_send[i] = btn_cmd[i];
       // }
     // DPRINTF("\n==== MODBUS CMD=====\n");
     /// TimerStart(&modbus_send_twice, 10);
     // TimerStart(&modbus_send_twice, 30);
     // TimerStart(&modbus_send_twice, 50);
     // TimerStart(&modbus_send_twice, 50);
 // }

      //sendFrame(btn_cmd);
     // led_came = false;
 // }
//  got_poll_cmd = false;
//  }
//  else{
//      DPRINTF("\n==== !!!!!!!!!!!!!!!!!!!!!=====\n");
//  }
}

//LED decoding function
void decode_led_command(uint16_t led_value) {
    // Extract individual LED states
    bool led1_state = (led_value & 0x0001) != 0;  // Bit 0
    bool led2_state = (led_value & 0x0002) != 0;  // Bit 1
    bool led3_state = (led_value & 0x0004) != 0;  // Bit 2
    bool led4_state = (led_value & 0x0008) != 0;  // Bit 3
    bool led5_state = (led_value & 0x0010) != 0;  // Bit 4
    bool led6_state = (led_value & 0x0020) != 0;  // Bit 5
    bool led7_state = (led_value & 0x0040) != 0;  // Bit 6
    bool led8_state = (led_value & 0x0080) != 0;  // Bit 7
    bool backlight_state = (led_value & 0x0100) != 0; // Bit 8

    DPRINTF("Value 0x%04X -> LEDs: %d%d%d%d%d%d%d%d Backlight: %d\n",
             led_value,
             led1_state, led2_state, led3_state, led4_state,
             led5_state, led6_state, led7_state, led8_state,
             backlight_state);

    Set_Led(1, !led1_state);
    Set_Led(2, !led2_state);
    Set_Led(3, !led3_state);
    Set_Led(4, !led4_state);
    Set_Led(5, !led5_state);
    Set_Led(6, !led6_state);
    Set_Led(7, !led7_state);
    Set_Led(8, !led8_state);
    set_proximity_level();
    make_leds_back_buff[0] = !led1_state;
    make_leds_back_buff[1] = !led2_state;
    make_leds_back_buff[2] = !led3_state;
    make_leds_back_buff[3] = !led4_state;
    make_leds_back_buff[4] = !led5_state;
    make_leds_back_buff[5] = !led6_state;
    make_leds_back_buff[6] = !led7_state;
    make_leds_back_buff[7] = !led8_state;
}

//LED command handler
void handle_led_command(uint8_t *modbus_frame) {
  led_came = true;
//  for(int i=0;i<FRAME_SIZE;i++){
//      DPRINTF("%02X ", modbus_frame[i]);
//  }
//  DPRINTF("\n");
    // Verify this is an LED command (function code 0x06 - Write Single Register)
    if (modbus_frame[1] != 0x06) {
        return; // Not an LED command
    }

    // Verify register address is LED control register (0x1008)
    if (modbus_frame[2] != 0x10 || modbus_frame[3] != 0x08) {
        return; // Not LED register
    }

    // Extract LED value from bytes 4-5
    uint16_t led_value = (modbus_frame[4] << 8) | modbus_frame[5];

    // Decode and set LEDs
    decode_led_command(led_value);

}

//Upon addr mode make all LEDs back to previuos state
void make_leds_back()
{
  for(int i=0;i<8;i++){
      Set_Led(i+1, make_leds_back_buff[i]);
  }
//  Set_Led(1, make_leds_back_buff[0]);
}
typedef struct{
  uint8_t keypad_Id;
}rx_addr_config_t;
rx_addr_config_t rx_data;

void timeout_config_addr_CB()
{
  if(addr_config_mode && enable_addr_mode)
  {
      DPRINT("\n Address Configuration Done\n");
      ApplicationData.KeypadID = defined_keypadID = rx_data.keypad_Id;
      writeAppData(&ApplicationData);

      //{keypad_Id, 0x46, 0x46, 0x46, 0x46, 0x46, crc1, crc2}; // Address Rx Confirm Config Command
      uint8_t configuration_save_ack[8] = {0xFF, 0x06, 0x10, 0x50, 0xFF, 0xFF, 0x99, 0x75};
      for(int i=0;i<5;i++)
        {
          sendFrame(configuration_save_ack);//Ack 5 times
          DPRINT("\n Addr Save Ack Sent\n");
        }
      TimerStop(&timeout_config_addr);
      DPRINT("\n=======Configured Data=========");
      DPRINTF("\nKeypad ID : %d\n", rx_data.keypad_Id);
      DPRINT("\n==================================");
      make_leds_back_buff[0] = 1;
      make_leds_back_buff[1] = 1;
      make_leds_back_buff[2] = 1;
      make_leds_back_buff[3] = 1;
      make_leds_back_buff[4] = 1;
      make_leds_back_buff[5] = 1;
      make_leds_back_buff[6] = 1;
      make_leds_back_buff[7] = 1;
      make_leds_back();
  }
  else
  {
      DPRINT("\n Address Configuration Not Done/Enabled\n");
  }
  addr_config_mode = enable_addr_mode = false;
  memset(Rx_Buff, 0, Rx_Buff_Len);
  TimerStop(&led_inc_toggle);
//  make_leds_back();
}
void process_rs485_rx_data()
{
  //01 06 10 08 01 01 cc 98 LED command
  if(Rx_Buff[0] == defined_keypadID && Rx_Buff[1] == 0x06 && Rx_Buff[2] == 0x10 && Rx_Buff[3] == 0x08)
    {
      DPRINT("\n ====LED command==== \n");
      handle_led_command(Rx_Buff);
      got_poll_cmd = false;
    }
 //uint8_t new_address[8] = {0xFF, 0x06, 0x10, 0x50, 0x00, panel_new_address, 0x00, 0x00};
  else if(Rx_Buff[0] == 0xFF && Rx_Buff[1] == 0x06 && Rx_Buff[2] == 0x10 && Rx_Buff[3] == 0x50 && Rx_Buff[7] != 0x75)// Address Rx Config Command
    {
      if(Rx_Buff[5] != 0){
        DPRINT("\n Ready for Address Configuration \n");
        //check CRC valid or not
        uint16_t crc = modbus_crc16(Rx_Buff, 6);
        uint8_t crc1 = crc & 0xFF;
        uint8_t crc2 = (crc >> 8) & 0xFF;
        if(crc1 == Rx_Buff[6] && crc2 == Rx_Buff[7])
        {
            rx_data.keypad_Id = Rx_Buff[5];
            enable_addr_mode = true;
            TimerStart(&led_inc_toggle, 50);
            TimerStart(&timeout_config_addr,60000);
        }
        else
          {
            DPRINT("\nConfig: ======CRC valid Error!======\n");
          }
      }
      else{
          DPRINT("\nConfig: ======Invalid Addr======\n");
      }
      got_poll_cmd = false;
    }
  //uint8_t target_ack[8] = {0xFF, 0x06, 0x10, 0x50, 0xFF, 0xFF, 0x99, 0x75};// Address Rx Confirm Config Command
  else if(Rx_Buff[0] == 0xFF && Rx_Buff[1] == 0x06 && Rx_Buff[6] == 0x99 && Rx_Buff[4] == 0xFF && Rx_Buff[7] == 0x75)
     {
      DPRINT("\n Rx Address Configuration Not Done/Enabled\n");
      uint16_t crc = modbus_crc16(Rx_Buff, 6);
      uint8_t crc1 = crc & 0xFF;
      uint8_t crc2 = (crc >> 8) & 0xFF;
      if(crc1 == Rx_Buff[6] && crc2 == Rx_Buff[7])
      {
          TimerStop(&timeout_config_addr);
          TimerStop(&led_inc_toggle);
          memset(Rx_Buff, 0, Rx_Buff_Len);
          addr_config_mode = enable_addr_mode = false;
          make_leds_back();
      }
      else
        {
          DPRINT("\n======Config Confirm: CRC valid Error!======\n");
        }
     // got_poll_cmd = false;
    }
//  else if(Rx_Buff[0] == 0xFD && Rx_Buff[1] == 0x03 && Rx_Buff[2] == 0x10 && Rx_Buff[3] == 0x0B
//      && Rx_Buff[4] == 0x00 && Rx_Buff[5] == 0x01)
//    {
////      for(int i=0;i<FRAME_SIZE;i++)
////        {
////          DPRINTF("%02X-", Rx_Buff[i]);
////        }
////      DPRINT("\n");
//  //    got_poll_cmd = true;
//    }
}
