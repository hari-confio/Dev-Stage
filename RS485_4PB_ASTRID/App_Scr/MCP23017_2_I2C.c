/*
 * MCP23017_I2C.c
 *
 *  Created on: 23-Nov-2023
 *      Author: Confio
 */

#include "app/framework/include/af.h"
#include "MCP23017_I2C.h"
#include "App_Scr/App_Main.h"
#include "MCP23017_2_I2C.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include <iic/driver_interface.h>

#include <stdint.h>
#include <stdbool.h>

//Keeping local ram variable to avoid i2c read operation
static uint8_t LOCAL_2_REG_PORT_A = 0x00;
static uint8_t LOCAL_2_REG_PORT_B = 0x00;

EmberEventControl Button_7_8_press_Control;


/*extern bool Button_7_State;
extern bool Button_8_State;
extern bool btn7Hold;
extern bool btn7Hold;
extern bool btn7LongHoldScheduled;
extern bool btn8LongHoldScheduled;
extern int btn7Cnt;
extern int btn8Cnt;*/
//int hold_btn = 0;


int btn7Count = 0, btn8Count = 0;
bool btn7Hold = false, btn8Hold = false, btn7Press = false, btn8Press = false;
//int btn7Cnt = 0,btn8Cnt = 0;
bool btn7LongHoldScheduled = false, btn8LongHoldScheduled = false;
uint32_t btn7PressTimestamp = 0;

int btn7PressCount = 0, btn8PressCount = 0;
bool btn7WaitingForDouble = false, btn8WaitingForDouble = false;


uint16_t prev_value_btn8 = 0xC0;
uint8_t press_count_btn8 = 0;
bool in_press_btn8 = false;
uint32_t press_duration_btn8 = 0;

uint16_t prev_value = 0xC0;
uint8_t press_count = 0;
bool in_press = false;
uint32_t press_duration = 0;

#define LONG_HOLD_THRESHOLD 5  // Adjust based on how long '0x40' persists

void Button_7_8_press_Handler()
{
  emberEventControlSetInactive(Button_7_8_press_Control);

  // Button 7
  if (btn7PressCount != 0) {
    if (btn7PressCount == 1) {
      emberAfCorePrintln("===Button7 Single Press===");
      start_timer_for_button_presses(7);
    } else if (btn7PressCount == 2) {
      emberAfCorePrintln("===Button7 Double Press===");
      start_timer_for_button_presses(7);
      start_timer_for_button_presses(7);
    }
    btn7PressCount = 0;
  }

  // Button 8
  if (btn8PressCount != 0) {
    if (btn8PressCount == 1) {
      emberAfCorePrintln("===Button8 Single Press===");
      start_timer_for_button_presses(8);
    } else if (btn8PressCount == 2) {
      emberAfCorePrintln("===Button8 Double Press===");
      start_timer_for_button_presses(8);
      start_timer_for_button_presses(8);
    }
    btn8PressCount = 0;
    btn8WaitingForDouble = false;
  }
}




void detect_button_event(uint8_t reg, uint8_t raw_value) {
    uint16_t current_value = raw_value & 0xF0;

    // Handle Button 7
    if (reg == 0x12 && (raw_value & 0xF0) == 0x40) {
        if (!in_press) {
            press_count++;
            in_press = true;
            press_duration = 1;
        } else {
            press_duration++;
        }
    }
    else if (reg == 0x12 && prev_value == 0x40 && (raw_value & 0xF0) == 0xC0) {
        if (press_duration >= LONG_HOLD_THRESHOLD) {
            emberAfCorePrintln("Button7 Long Hold");
            start_timer_for_button_presses(110);
        } else if (press_count == 1) {
            btn7PressCount++;
        } else if (press_count == 2) {
            btn7PressCount++;
        }
        emberEventControlSetDelayMS(Button_7_8_press_Control, 100);
        press_count = 0;
        press_duration = 0;
        in_press = false;
    }
    prev_value = current_value;

    // Handle Button 8
    if (reg == 0x12 && raw_value == 0x80) {
        if (!in_press_btn8) {
            press_count_btn8++;
            in_press_btn8 = true;
            press_duration_btn8 = 1;
        } else {
            press_duration_btn8++;
        }
    }
    else if (reg == 0x12 && prev_value_btn8 == 0x80 && raw_value == 0xC0) {
        if (press_duration_btn8 >= LONG_HOLD_THRESHOLD) {
            emberAfCorePrintln("Button8 Long Hold");
            start_timer_for_button_presses(120);
        } else if (press_count_btn8 == 1) {
            btn8PressCount++;
        } else if (press_count_btn8 == 2) {
            btn8PressCount++;
        }
        emberEventControlSetDelayMS(Button_7_8_press_Control, 100);
        press_count_btn8 = 0;
        press_duration_btn8 = 0;
        in_press_btn8 = false;
    }

    prev_value_btn8 = raw_value;
}





void MCP23017_2_readReg(uint8_t reg)
{
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;

  //uint8_t i2c_txBuffer[1] = {reg};
  //uint8_t i2c_txBufferSize = sizeof(i2c_txBuffer);
  uint8_t i2c_rxBuffer[1] = {0};
  //uint8_t i2c_rxBufferIndex;

  // Initializing I2C transfer
  i2cTransfer.addr          = MCP23017_ADDRESS_2;
  i2cTransfer.flags         = I2C_FLAG_WRITE_READ;
  i2cTransfer.buf[0].data   = &reg;
  i2cTransfer.buf[0].len    = 1;
  i2cTransfer.buf[1].data   = i2c_rxBuffer;
  i2cTransfer.buf[1].len    = 1;
  result = I2C_TransferInit(I2C0, &i2cTransfer);

  // Sending data
  while (result == i2cTransferInProgress)
  {
    result = I2C_Transfer(I2C0);
  }
 // uint16_t current_value = i2c_rxBuffer[0] & 0x00F0;  // Masking to get relevant bits
  detect_button_event(reg, i2c_rxBuffer[0]);

 // emberAfCorePrintln("err %d\n", result);
//  if(i2c_rxBuffer[0] != 0)
 //   {
  //emberAfCorePrintln("MCP23017_2_readReg.... %02X  Done   %02X\n",reg, i2c_rxBuffer[0]);
 // if (reg == 0x0012 && i2c_rxBuffer[0] == 0x40)
  //{
//      btn7PressCount++;
//
//      if (!btn7WaitingForDouble)
//      {
//          btn7WaitingForDouble = true;
//          emberEventControlSetDelayMS(Button_7_8_press_Control, 1000); // Wait 400ms for possible second press
//      }
//
//      emberAfCorePrintln("===Button7 Pressed=== Count: %d", btn7PressCount);
 // }



//    if(reg == 0x0012 && i2c_rxBuffer[0] == 0x0080)
//      {
//        btn8Count++;
//        if(btn8Count >= 2)
//          {
//            emberAfCorePrintln("\n===Button8 Pressed====");
//                  start_timer_for_button_presses(8);
//                  //send_zigbee_button_event(8, 130);
//                  btn8Count = 0;
//                  //sendFrame(8);
//          }
//    }
}

void GpioStatePrintTask()
{
  uint8_t btn7State = 0, btn8State = 0;
  MCP23017_2_GetPinStates(&btn7State, &btn8State);
  emberAfCorePrintln("Initial Pin 7: %d, Initial Pin 8: %d\n", btn7State, btn8State);
 // while(1)
  //  {
//      uint8_t currentPin7State, currentPin8State;
   //   MCP23017_2_GetPinStates(&currentPin7State, &currentPin8State);
   // }
}
void MCP23017_2_writeReg(uint8_t reg, uint8_t value)
{
  interface_iic_write(MCP23017_ADDRESS_2, reg, &value, 1);
//  emberAfCorePrintln("reg %d\n", reg);
 // emberAfCorePrintln("value %d\n", value);
}

void MCP23017_2_GPIO_PinOutToggle(uint8_t port, uint8_t pin)
{
  uint8_t mask = 1 << pin;
  if(port == MCP23017_GPIO_A)
  {
    LOCAL_2_REG_PORT_A = (uint8_t)(LOCAL_2_REG_PORT_A ^ mask);
    MCP23017_2_writeReg(MCP23017_GPIO_A,LOCAL_2_REG_PORT_A);
  }
  if(port == MCP23017_GPIO_B)
  {
    LOCAL_2_REG_PORT_B = (uint8_t)(LOCAL_2_REG_PORT_B ^ mask);
    MCP23017_2_writeReg(MCP23017_GPIO_B,LOCAL_2_REG_PORT_B);
  }
}
void MCP23017_2_GPIO_PinOutSet(uint8_t port, uint8_t pin, uint8_t value)
{
  uint8_t mask = 1 << pin;
  if(port == MCP23017_GPIO_A)
  {
    LOCAL_2_REG_PORT_A = value?(uint8_t)(LOCAL_2_REG_PORT_A | mask):(uint8_t)(LOCAL_2_REG_PORT_A & ~mask);
    MCP23017_2_writeReg(MCP23017_GPIO_A,LOCAL_2_REG_PORT_A);
  }
  if(port == MCP23017_GPIO_B)
  {
    LOCAL_2_REG_PORT_B = value?(uint8_t)(LOCAL_2_REG_PORT_B | mask):(uint8_t)(LOCAL_2_REG_PORT_B & ~mask);
    MCP23017_2_writeReg(MCP23017_GPIO_B,LOCAL_2_REG_PORT_B);
  }
}


void MCP23017_2_Init()
{
  emberAfCorePrintln("MCP23017_2_Init......");
  //  disable address increment (datasheet)
  //MCP23017_writeReg(MCP23017_IOCR, 0b00100000);
  //  Force INPUT_PULLUP
  MCP23017_2_writeReg(MCP23017_PUR_A, 0xFF);
  MCP23017_2_writeReg(MCP23017_PUR_B, 0xFF);

  //  Ports Direction bit 0-Output,  bit 1-Input
  MCP23017_2_writeReg(MCP23017_DDR_A, 0x00);
  MCP23017_2_writeReg(MCP23017_DDR_B, 0x00);

  LOCAL_2_REG_PORT_A = 0x00;
  LOCAL_2_REG_PORT_B = 0x00;

  MCP23017_2_writeReg(MCP23017_GPIO_A,LOCAL_2_REG_PORT_A);
  MCP23017_2_writeReg(MCP23017_GPIO_B,LOCAL_2_REG_PORT_B);
//
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_1_PORT, APP_RELAY_1_PIN, 0);
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_2_PORT, APP_RELAY_2_PIN, 0);
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_3_PORT, APP_RELAY_3_PIN, 0);
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_4_PORT, APP_RELAY_4_PIN, 0);
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_5_PORT, APP_RELAY_5_PIN, 0);
//  MCP23017_2_GPIO_PinOutSet(APP_RELAY_6_PORT, APP_RELAY_6_PIN, 0);

 // MCP23017_2_GPIO_PinOutSet(APP_RELAY_7_PORT, APP_RELAY_7_PIN, 1);
  MCP23017_2_GPIO_PinOutSet(APP_RELAY_8_PORT, APP_RELAY_8_PIN, 1);
  MCP23017_2_GPIO_PinOutSet(APP_RELAY_7_PORT, APP_RELAY_7_PIN, 1);

//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 0, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 1, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 2, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 3, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 4, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 5, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 6, 0);
//  MCP23017_2_GPIO_PinOutSet(MCP23017_GPIO_A, 7, 0);

  emberAfCorePrintln("MCP23017_2_Init......Done !!!");

 // GpioStatePrintTask();
}


//void MCP23017_2_GetPinStates(uint8_t *pin7State, uint8_t *pin8State) {
//    uint8_t gpioState = MCP23017_2_readReg(MCP23017_GPIO_A);
//  //  DPRINTF("MCP23017_GPIO_A Register Value: 0x%02X\n", gpioState);
//
//    *pin7State = (gpioState >> APP_RELAY_7_PIN) & 0x01;
//    *pin8State = (gpioState >> APP_RELAY_8_PIN) & 0x01;
//}
