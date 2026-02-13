/*
 * App_Main.c
 *
 *  Created on: 30-Jan-2023
 *      Author: Confio
 */





/*


Device Inclusion = 4 times press 3rd Scene button

Device Exclude/Reset = 13 times press 3rd Scene button


*/







#include "App_Src/App_Main.h"
#include "App_Src/App_StatusUpdate.h"

#include <em_usart.h>
#include <stdbool.h>
#include "bsp.h"
#include "em_timer.h"

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
uint8_t buffer[RX_BUFF_LEN];

uint16_t Boot_Count = 0;
uint8_t Broadcast_seq = 1;
uint8_t Button_3_Count = 0;
uint8_t BUTTON_STATUS;

uint8_t rotaryLevel = 0;
bool receive = true;

uint8_t MACID[8];
EmberEventControl Button_3_Counter_Control;
EmberEventControl appBroadcastSearchTypeCT3B1REventControl;

void sendFrame(uint8_t *data, size_t dataSize);
void encryptAndSendData(uint32_t *data, int length);

#define RX_BUFF_LEN 64

volatile uint16_t Rx_Buff[RX_BUFF_LEN] = {0};
static uint16_t Rx_Buff_Len = 0;
#define RX_BUFF_SIZE 128

/////////////////////////////////////////////////////////////////////////////////
/// Device Inclusion Exclusion counter Handler
/////////////////////////////////////////////////////////////////////////////////

uint8_t active_button = 0;
uint8_t active_count = 0;
uint8_t active_level = 0;
uint8_t scene_executing = 0;
volatile uint8_t knobReport = 0;
uint8_t btn1Level = 0, btn2Level = 0, btn3Level = 0;
uint32_t btn1Buff[14], btn2Buff[14], btn3Buff[14];
int inc_exc_cnt = 0, inc_exc_cnt1 = 0;

void Button_3_Counter_Handler()
{
  emberEventControlSetInactive(Button_3_Counter_Control);
  emberAfCorePrintln("Button_Counter_Handler %d %d %d", active_button, active_count, active_level);
  emberAfCorePrintln("inc_exc_cnt %d %d", inc_exc_cnt, inc_exc_cnt1);
//  if(active_button == 1 && active_count == 4)
//    {
//      if (emberAfNetworkState() == EMBER_NO_NETWORK)
//        {
//      emberAfPluginNetworkSteeringStart ();
//      emberAfCorePrintln("Network Steering Started");
//        }
//    }
//
//  if(active_button == 1 && active_count >= 13)
//    {
//      if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
//        {
//            emberLeaveNetwork();
//             emberAfCorePrintln("Leaving Network");
//        }
//    }
//
//  if(active_button == 1)
//    {
//      BUTTON_STATUS = active_count;
//      emberEventControlSetDelayMS(app_Button_1_StatusUpdateEventControl, 200);
//    }
//  if(active_button == 2)
//    {
//      BUTTON_STATUS = active_count;
//      emberEventControlSetDelayMS(app_Button_2_StatusUpdateEventControl, 200);
//    }
//  if(active_button == 3)
//    {
//      BUTTON_STATUS = active_count;
//      emberEventControlSetDelayMS(app_Button_3_StatusUpdateEventControl, 200);
//    }
//
//  if(active_button == 4)
//     {
//      if(knobReport == 1)
//      {
//          knobReport = 0;
//      }
//      else
//      {
//          BUTTON_STATUS = active_level;
//          emberEventControlSetDelayMS(app_Button_Rotatory_StatusUpdateEventControl, 200);
//      }
//     }

  if (active_button == 1 && inc_exc_cnt == inc_exc_cnt1 && active_count >= 3 && active_count <= 7)
      {
        emberAfCorePrintln("----Inclusion----\n");
        btn1Buff[10] = 4;
        create_frame(1, 0); // 0 indicates for Inclusion/Exclusion
      }
    else if (active_button == 1 && inc_exc_cnt == inc_exc_cnt1 && active_count >= 10)
      {
        emberAfCorePrintln("----Exclusion----\n");
        btn1Buff[10] = 13;
        create_frame(1, 0); // 0 indicates for Inclusion/Exclusion

      }
    else if(active_button == 1)
      {
        emberAfCorePrintln("----Btn-%d : Level-%d---\n", active_button, btn1Level);
        btn1Buff[9] = 1;
        btn1Buff[10] = 1;
        create_frame(1, btn1Level);

      }
    else if(active_button == 2)
      {
        emberAfCorePrintln("----Btn-%d : Level-%d---\n", active_button, btn2Level);
        btn2Buff[9] = 2;
        btn2Buff[10] = 1;
        create_frame(2, btn2Level);

      }
    else if(active_button == 3)
      {
        emberAfCorePrintln("----Btn-%d : Level-%d---\n", active_button, btn3Level);
        btn3Buff[9] = 3;
        btn3Buff[10] = 1;
        create_frame(3, btn3Level);

      }
  active_button = 0;
  active_count = 0;
  active_level = 0;
  inc_exc_cnt = 0;
  inc_exc_cnt1 = 0;
}


void button_presses(uint8_t button)
{
  static uint8_t prv_button = 0;
  active_button = button;
  active_count++;
  if(prv_button != active_button)
    {
      prv_button = active_button;
      active_count = 1;
    }

  emberEventControlSetDelayMS(Button_3_Counter_Control, 500);

}




/////////////////////////////////////////////////////////////////////////////////
/// Processing the data received from the device
/////////////////////////////////////////////////////////////////////////////////

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



void create_frame(int button, int Rotary_val)
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

  activeBuff[8] = 0x03; // ATLAS
 // emberAfCorePrint("activeBuff[10]-%X", activeBuff[10]);
  activeBuff[9] = button;
  //activeBuff[10] = Rotary_val;
  activeBuff[11] = Rotary_val;
 // activeBuff[10] = btn_status;
  uint16_t crc = calculate_modbus_crc(activeBuff, 12);

  activeBuff[12] = crc & 0xFF;
  activeBuff[13] = (crc >> 8) & 0xFF;

 // emberAfCorePrintln("-----BTN-%d-action-%d--> FRAME :\n", button, Rotary_val);
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
        USART_Tx(USART2, encryptedFrame[i]);
        emberAfCorePrint("%02X ", encryptedFrame[i]);
    }
    emberAfCorePrint("\n");

}


/////////////////////////////////////////////////////////////////////////////////
/// Interrupt Handler to receive the data from the device using USART protocol
/////////////////////////////////////////////////////////////////////////////////
void USART1_RX_IRQHandler(void)
{
  uint32_t flags;
    flags = USART_IntGet(USART1);
    USART_IntClear(USART1, flags);

    uint8_t data;
//
//    //start a timer(HW timer)
    TIMER_CounterSet(TIMER1,1);
    TIMER_Enable(TIMER1,true);

    data = USART1->RXDATA;
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

#define RX_BUFF_LEN1 64
volatile uint16_t Rx_Buff1[RX_BUFF_LEN1] = {0};
static uint16_t Rx_Buff_Len1 = 0;

void USART2_RX_IRQHandler(void)
{
//  emberAfCorePrintln("USART2_RX_IRQHandler");
  uint32_t flags;
  flags = USART_IntGet(USART2);
  USART_IntClear(USART2, flags);

  uint32_t data;

  // Start a timer (HW timer)
  TIMER_CounterSet(TIMER2, 1);
  TIMER_Enable(TIMER2, true);

  data = (uint32_t)USART_Rx(USART2);
  if (Rx_Buff_Len1 < RX_BUFF_SIZE) // Ensure no overflow
  {
      Rx_Buff1[Rx_Buff_Len1++] = data;
      //emberAfCorePrintln("%X ", data);
  }
  else
  {
      emberAfCorePrintln("Rx_Buff overflow");
  }
}

void USART2_TX_IRQHandler(void)
{
    uint32_t flags;
    flags = USART_IntGet(USART2);
    USART_IntClear(USART2, flags);
    if (flags & USART_IF_TXC)
    {
        //emberAfCorePrintln("\nTransmission completed");
    }
    GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);
    GPIO_PinOutClear(READ_PORT, READ_PIN);
}
/////////////////////////////////////////////////////////////////////////////////
/// Interrupt Handler for the Hardware Timer, it called when Hw Timer initialized
/////////////////////////////////////////////////////////////////////////////////
void TIMER1_IRQHandler(void)
{
  //emberAfCorePrintln("TIMER1_IRQHandler");
  uint32_t flags = TIMER_IntGet(TIMER1);
  TIMER_IntClear(TIMER1, flags);

  TIMER_Enable(TIMER1,false);


  processReceivedData();
  Rx_Buff_Len = 0;

}

void TIMER2_IRQHandler(void)
{
  uint32_t flags = TIMER_IntGet(TIMER2);
  TIMER_IntClear(TIMER2, flags);

  TIMER_Enable(TIMER2,false);

  RS485ReceivedData();
  Rx_Buff_Len1 = 0;

}

void RS485ReceivedData()
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

 // 84 BA 20 FF FE 92 5E 5F 01 01 01 62 D0
//1-8(MAC),9-5(RS485)

for(int i=0; i<8; i++)
  {
    if(Rx_Buff1[i] == MACID[i])
      {
        mac_match_cnt++;
        if(mac_match_cnt == 7)
          {
            // Proceed for further processing
            if(Rx_Buff1[8] == 0x01)
              {
                if(Rx_Buff1[9] == 0x04)//  ----Device Inclusion----
                  {
//                    led_set_to_white();
                  }
                else if (Rx_Buff1[9] == 0x0D) // ---- Device Exclusion ----
                {
                   // led_set_to_white();

                }
                 else if(Rx_Buff1[9] == 0x01)
                 {
                    // ledState11 = !ledState11;  // Toggle LED state for Button == 1 & SinglePress
                   //  set_led(1, Rx_Buff[10]);
//                     emberAfCorePrintln("--Response--BTN - %d\n", 1);
                 }
                 else if(Rx_Buff1[9] == 0x02)
                 {
                    // ledState12 = !ledState12;  // Toggle LED state for Button == 1 & DoublePress
                     //set_led(1, Rx_Buff[10]);
                 }
                 else if(Rx_Buff1[9] == 0x03)
                 {
                   //  ledState1Long = !ledState1Long;  // Toggle LED state for for Button == 1 & long hold
                     //set_led(1, Rx_Buff[10]);
                 }
              }
            else if(Rx_Buff1[8] == 0x02)
              {
                 if(Rx_Buff1[9] == 0x01)
                 {
                    // ledState21 = !ledState21;
                     //set_led(2, Rx_Buff[10]);
                 }
                 else if(Rx_Buff1[9] == 0x02)
                 {
                    // ledState22 = !ledState22;
                     //set_led(2, Rx_Buff[10]);
                 }
                 else if(Rx_Buff1[9] == 0x03)
                 {
                   //  ledState2Long = !ledState2Long;
                    // set_led(2, Rx_Buff[10]);
                 }
              }
            else if(Rx_Buff1[8] == 0x03)
              {
                 if(Rx_Buff1[9] == 0x01)
                 {
                    // ledState31 = !ledState31;
                     //set_led(3, Rx_Buff[10]);
                 }
                 else if(Rx_Buff1[9] == 0x02)
                 {
                    // ledState32 = !ledState32;
                    // set_led(3, Rx_Buff[10]);
                 }
                 else if(Rx_Buff1[9] == 0x03)
                 {
                    // ledState3Long = !ledState3Long;
                    // set_led(3, Rx_Buff[10]);
                 }
              }

      }

      }

    else
    {
        emberAfCorePrintln("------MAC ID Not Matched----\n");
    }
  }
  }


    memset(Rx_Buff1, 0, sizeof(Rx_Buff1)); // Clear buffer
    Rx_Buff_Len1 = 0; // Reset length

}


void processReceivedData()
{

 // if(Rx_Buff[0] != 0)
 // {
//  emberAfCorePrintln("--Response--\n");
//  for(int i=0; i<Rx_Buff_Len; i++)
//    {
//      emberAfCorePrint("%X ", Rx_Buff[i]);
//    }
  //}

  // Determine the endpoint (ep) from Rx_Buff[8] using switch-case
  uint8_t ep = 0;
  switch (Rx_Buff[8])
  {
      case 0x33: ep = 1; break;
      case 0x34: ep = 2; break;
      case 0x35: ep = 3; break;
      default:
        emberAfCorePrint("Invalid EP in Rx_Buff[8]: %02X\n", Rx_Buff[8]);
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
              emberAfCorePrint("Unexpected data length for dimming frame: %02X\n", Rx_Buff[16]);
              break;
          }
          // Now check Rx_Buff[13] to distinguish normal dimming vs. color temperature
          switch(Rx_Buff[13])
          {
            case 0x01: // Curtain stop command
            {
                if (Rx_Buff[20] == 0x70)
                {
                    emberAfCorePrint("CURTAIN STOP: EP=%d\n", ep);
                    if(ep == 1)
                      {
                        btn1Level = 0x65; //65 INDICATES STOP COMMAND
                      }
                    else if(ep == 2)
                      {
                        btn2Level = 0x65;
                      }
                    else if(ep == 3)
                      {
                        btn3Level = 0x65;
                      }
                    active_level = 0x65;
                    button_presses(ep);

                }


            }
            break;
            case 0x02: // Curtain control (direct decimal percentage)
            {
                uint8_t databyte = Rx_Buff[20];       // e.g., 0x64 will be 100
                int final_percent = (databyte > 100) ? 100 : databyte;  // Cap at 100

                emberAfCorePrint("CURTAIN : EP=%d, Calculated Percentage=%d%%\n", ep, final_percent, final_percent);
                if(ep == 1)
                  {
                    btn1Level = final_percent;
                  }
                else if(ep == 2)
                  {
                    btn2Level = final_percent;
                  }
                else if(ep == 3)
                  {
                    btn3Level = final_percent;
                  }
                active_level = final_percent;
                button_presses(ep);
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
                        emberAfCorePrint("Invalid unit digit for normal dimming: %d\n", units);
                          break;
                  }
                  if(base_percent == -1)
                      break;

                  int increment = remaining / 10;  // +1% per full 10 in remaining digits
                  int final_percent = base_percent + increment;
                  if(final_percent > 100)
                      final_percent = 100;

                  emberAfCorePrint("Normal Dimming: EP=%d, Calculated Percentage=%d%%\n", ep, final_percent);
                 // Button_Status(ep, final_percent);
                  inc_exc_cnt++;
                  if(ep == 1)
                    {
                      btn1Level = final_percent;
                    }
                  else if(ep == 2)
                    {
                      btn2Level = final_percent;
                    }
                  else if(ep == 3)
                    {
                      btn3Level = final_percent;
                    }
                  active_level = final_percent;
                  button_presses(ep);
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
                        emberAfCorePrint("Invalid unit digit for color temperature: %d\n", units);
                          break;
                  }
                  if(base_value == -1)
                      break;

                  int increment = remaining / 10;
                  int final_value = base_value + increment;
                  if(final_value > 100)
                      final_value = 100;

                  emberAfCorePrint("Color Temperature: EP=%d, Calculated Value=%d\n", ep, final_value);
                  inc_exc_cnt1++;
                //  Set_ColorTemp(ep, final_value);
              }
              break;

              default:
                emberAfCorePrint("Invalid payload type in dimming frame (Rx_Buff[13]): %02X\n", Rx_Buff[13]);
                  break;
          }
      }
      break;

      case 0x0A:  // Off frame
      {
          // Verify that data length (Rx_Buff[16]) is expected (0x01)
          if(Rx_Buff[16] != 0x01) {
              emberAfCorePrint("Unexpected data length for off frame: %02X\n", Rx_Buff[16]);
              break;
          }
          // Use Rx_Buff[17] as the off value
          if(Rx_Buff[17] == 0x00) {
          uint8_t off_val = Rx_Buff[17];
          emberAfCorePrint("Off Frame: EP=%d, Off Value=%02X\n", ep, off_val);
         // Button_Status(ep, off_val);
          if(ep == 1)
            {
              btn1Level = off_val;
            }
          else if(ep == 2)
            {
              btn2Level = off_val;
            }
          else if(ep == 3)
            {
              btn3Level = off_val;
            }
          active_level = off_val;
          button_presses(ep);
          }
      }
      break;
      case 0x0E:
        {
         if (Rx_Buff[21] == 0x65)
           {
             emberAfCorePrint("CURTAIN OFF: EP=%d\n", ep);
           }
         if(ep == 1)
           {
             btn1Level = 0;
           }
         else if(ep == 2)
           {
             btn2Level = 0;
           }
         else if(ep == 3)
           {
             btn3Level = 0;
           }
         active_level = 0;
         button_presses(ep);
        }
        break;
      default:
        emberAfCorePrint("Invalid frame type in Rx_Buff[7]: %02X\n", Rx_Buff[7]);
          break;
  }

cleanup:
  // Reset the buffer length for the next frame
  Rx_Buff_Len = 0;
}

//void Set_ColorTemp(uint8_t ep, int val)
//{
//    // Print the color temperature settings.
//  emberAfCorePrint("Color Temp: EP=%d, Value=%d\n", ep, val);
//}

uint16_t count = 0;
uint16_t count1 = 0;
uint16_t count2 = 0;
uint8_t hexValue=0;


void Button_Status(uint8_t ep, uint8_t val)
{

  if(ep == 1)
  {
    count++;

    emberAfCorePrintln("Button_Status(%d, %d%%)\n", ep, val); // Replace with actual function implementation
    rotaryLevel = val;
     hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);

     btn1Buff[10] = 1;
     create_frame(1, val);

  }
  if(ep == 2)
  {
      emberAfCorePrintln("Button_Status(%d, %d%%)\n", ep, val); // Replace with actual function implementation
      count1++;
         rotaryLevel = val;
           hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);
     //  emberAfCorePrintln("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
           btn2Buff[10] = 1;
           create_frame(2, val);
  }
  if(ep == 3)
  {
      emberAfCorePrintln("Button_Status(%d, %d%%)\n", ep, val); // Replace with actual function implementation
      count2++;
        rotaryLevel = val;
          hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);
   //   emberAfCorePrintln("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);

          btn3Buff[10] = 1;
          create_frame(3, val);
  }

}

/////////////////////////////////////////////////////////////////////////////////
/// Initializing the Hardware timer pin of the device.
/////////////////////////////////////////////////////////////////////////////////
void Init_Hw_Timer()
{
  emberAfCorePrintln("====Init_Hw_Timer");

    CMU_ClockEnable(cmuClock_TIMER1, true);

    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
    timerCCInit.mode = timerCCModeCompare;
    TIMER_InitCC(TIMER1, 0, &timerCCInit);

    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    timerInit.prescale = timerPrescale4 ;
    timerInit.enable = false;

    TIMER_Init(TIMER1, &timerInit);
    TIMER_TopSet(TIMER1, 0xFFFF);


    TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
    NVIC_EnableIRQ(TIMER1_IRQn);

}


void Init_Hw_Timer2()
{
    emberAfCorePrintln("====Init_Hw_Timer2");

    CMU_ClockEnable(cmuClock_TIMER2, true);

    TIMER_InitCC_TypeDef timerCCInit1 = TIMER_INITCC_DEFAULT;
    timerCCInit1.mode = timerCCModeCompare;
    TIMER_InitCC(TIMER2, 0, &timerCCInit1);

    TIMER_Init_TypeDef timerInit1 = TIMER_INIT_DEFAULT;
    timerInit1.prescale = timerPrescale4;
    timerInit1.enable = false;

    TIMER_Init(TIMER2, &timerInit1);
    TIMER_TopSet(TIMER2, 0xFFFF);

    TIMER_IntEnable(TIMER2, TIMER_IEN_CC0);
    NVIC_EnableIRQ(TIMER2_IRQn);

}


/////////////////////////////////////////////////////////////////////////////////
/// Initializing the USART pin of the device.
/////////////////////////////////////////////////////////////////////////////////
void Serial_Init()
{
    CMU_ClockEnable(cmuClock_USART1, true);

    USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

    //set pin mode for USART TX and RX pins
    GPIO_PinModeSet(SERIAL_RX_PORT, SERIAL_RX_PIN, gpioModeInput, 0);              // Rx
    GPIO_PinModeSet(SERIAL_TX_PORT, SERIAL_TX_PIN, gpioModePushPull, 1);           // TX

    //Initialize USART asynchronous mode and route pins

    GPIO->USARTROUTE[1].RXROUTE = (SERIAL_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
          | (SERIAL_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[1].TXROUTE = (SERIAL_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
          | (SERIAL_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);

    // Enable RX and TX signals now that they have been routed
     GPIO->USARTROUTE[1].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

    // Configure and enable USART1
     USART_InitAsync(USART1, &init);

     //Initialize USART Interrupts
     USART_IntEnable(USART1, USART_IEN_RXDATAV);
     USART_IntEnable(USART1, USART_IEN_TXC);

     //Enable NVIC USART sources
     NVIC_ClearPendingIRQ(USART1_RX_IRQn);
     NVIC_EnableIRQ(USART1_RX_IRQn);

     //NVIC_ClearPendingIRQ(USART1_TX_IRQn);
     //NVIC_EnableIRQ(USART1_TX_IRQn);

     Init_Hw_Timer();
}
void Serial_Init_USART2()
{
    CMU_ClockEnable(cmuClock_USART2, true);

    USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
    init.baudrate = 9600;
    // Set pin mode for USART TX and RX pins for USART2
   // GPIO_PinModeSet(USART2_RX_PORT, USART2_RX_PIN, gpioModeInput, 0);   // Rx
    GPIO_PinModeSet(USART2_RX_PORT, USART2_RX_PIN, gpioModeInputPull, 0);   // RX with pull-down

    GPIO_PinModeSet(USART2_TX_PORT, USART2_TX_PIN, gpioModePushPull, 1); // TX
    GPIO_PinModeSet(READ_PORT, READ_PIN, gpioModePushPull, 0);  // Control GPIO
    GPIO_PinModeSet(WRITE_PORT, WRITE_PIN, gpioModePushPull, 0); // Control GPIO

    GPIO_PinOutClear(READ_PORT, READ_PIN);
    GPIO_PinOutClear(WRITE_PORT, WRITE_PIN);
//    GPIO_PinOutClear(USART2_RX_PORT, USART2_RX_PIN);
 //     GPIO_PinOutClear(USART2_TX_PORT, USART2_TX_PIN);
    // Initialize USART asynchronous mode and route pins for USART2
    GPIO->USARTROUTE[2].RXROUTE = (USART2_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
          | (USART2_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[2].TXROUTE = (USART2_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
          | (USART2_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);

    // Enable RX and TX signals now that they have been routed for USART2
    GPIO->USARTROUTE[2].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

    // Configure and enable USART2
    USART_InitAsync(USART2, &init);

    // Enable RX and TX interrupts for USART2 and USART1
    USART_IntEnable(USART2, USART_IEN_RXDATAV);
    NVIC_ClearPendingIRQ(USART2_RX_IRQn);
    NVIC_EnableIRQ(USART2_RX_IRQn);

    USART_IntEnable(USART2, USART_IEN_TXC);
    NVIC_ClearPendingIRQ(USART2_TX_IRQn);
    NVIC_EnableIRQ(USART2_TX_IRQn);

    // Initialize the hardware timer
    Init_Hw_Timer2();
}

/////////////////////////////////////////////////////////////////////////////////
///        Starting the Broadcasting of the device to connect with network    ///
/// /////////////////////////////////////////////////////////////////////////////
void appBroadcastSearchTypeCT3B1REventHandler()
{
  emberEventControlSetInactive(appBroadcastSearchTypeCT3B1REventControl);

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

          0x0C,                      // Length
          0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x41, 0x54, 0x4C, 0x41, 0x53,    //(Confio:ATLAS)

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

bool  emberAfStackStatusCallback (EmberStatus status)
{
  emberAfCorePrintln(" ===========in status callback \n");
      if (status == EMBER_NETWORK_DOWN && emberAfNetworkState() == EMBER_NO_NETWORK)
       {
//         emAfPluginScenesServerClear();
//         emAfGroupsServerCliClear();
       }
     if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
       {
//         Network_open = false;
         emberAfCorePrintln(" ===========in status callback \n");
         emberEventControlSetDelayMS(appBroadcastSearchTypeCT3B1REventControl,500);
         emberEventControlSetDelayMS(app3B1RAnnouncementEventControl,2000);
       }

//     LED_BlinkTimeout = false;
//     emberEventControlSetActive(commissioningLedEventControl);

  return false;
}

bool emberAfPluginScenesServerCustomRecallSceneCallback (
    const EmberAfSceneTableEntry *const sceneEntry, uint16_t transitionTimeDs,
    EmberAfStatus *const status)
{
//      Relay_Operation(sceneEntry->endpoint,sceneEntry->onOffValue);
  return true;
}


/////////////////////////////////////////////////////////////////////////////////
/// Callback Called when the new data is received.s
/////////////////////////////////////////////////////////////////////////////////
///
uint8_t updateKnobLevel = 0;
boolean emberAfPreCommandReceivedCallback (EmberAfClusterCommand *cmd)
{
  uint8_t attributeId = cmd->buffer[3];


  uint8_t data1 = 0;
  uint8_t data2 = 0;
  if(cmd->bufLen>8)
    {
      data1 = cmd->buffer[8];
    }
  if(cmd->bufLen>9)
     {
       data2 = cmd->buffer[9];
     }

  emberAfCorePrintln("emberAfPreCommandReceivedCallback %2X %2X %2X\n", data1, data2, cmd->apsFrame->clusterId);


  if(cmd->apsFrame->clusterId == 0xFC00 && data2 == 4)
    {
      emberAfCorePrintln("Update Rotary Knob = %d",data1);
      updateKnobLevel = data1;
      emberEventControlSetDelayMS(app_Button_Rotatory_UpdateEventControl, 100);
    }

  if (cmd->apsFrame->clusterId == 0x0001 && cmd->commandId == 0)
    {
      if (attributeId == 3) // replay to controller announcing attributes
        {
          emberEventControlSetActive(app3B1RAnnouncementEventControl);
        }
    }

  return false;
}




void app_Button_Rotatory_UpdateEventHandler()
{
   emberEventControlSetInactive(app_Button_Rotatory_UpdateEventControl);
   emberAfCorePrintln("app_Button_Rotatory_UpdateEventHandler");
   static uint16_t seq = 0;

    uint8_t State_buf[17] = {0x55, 0xAA, 0x02, 0x00, 0x03, 0x04, 0x00, 0x08, 0x71, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x32, 0xB9};
    State_buf[15] = updateKnobLevel;
    uint16_t Sum = 0;
    State_buf[3] = ((seq & 0xFF00)>>0x08);
    State_buf[4] = (seq & 0x00FF);
    for(uint16_t i=0; i<16; i++)
    {
      Sum = Sum + State_buf[i];
    }
    State_buf[16] = (uint8_t)((Sum%256) & 0xFF);

    emberAfCorePrint("Knb Level TX - >");
    for(uint16_t i=0; i<17; i++)
    {
        emberAfCorePrint("%X ",State_buf[i]);
    }
    emberAfCorePrintln("\n");

    knobReport = 1;
    for(uint16_t i = 0; i<17; i++)
    {
      USART_Tx(USART1, State_buf[i]);
    }
    seq++;
}

void emberAfMainInitCallback(void)
{
 //emberEventControlSetActive(commissioningLedEventControl);

  halCommonGetToken(&Boot_Count, TOKEN_BOOT_COUNT_1);
  Boot_Count++;
  int value[1]={Boot_Count};
  emberAfWriteManufacturerSpecificServerAttribute(Endpoint_1, ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                     0x000D, EMBER_AF_MANUFACTURER_CODE, (uint8_t*)value, ZCL_INT32U_ATTRIBUTE_TYPE);
  emberAfCorePrintln("====Boot_Count :  %d  ", Boot_Count);
  Serial_Init();
  Serial_Init_USART2();

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
//      emberAfCorePrint("%X", btn6Buff[i]);
  }
  emberAfCorePrintln("");
}
