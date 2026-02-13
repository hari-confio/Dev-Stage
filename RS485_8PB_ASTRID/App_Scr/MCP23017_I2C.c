/*
 * MCP23017_I2C.c
 *
 *  Created on: 23-Nov-2023
 *      Author: Confio
 */

#include "app/framework/include/af.h"
#include "MCP23017_I2C.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include <iic/driver_interface.h>


//Keeping local ram variable to avoid i2c read operation
static uint8_t LOCAL_REG_PORT_A = 0x00;
static uint8_t LOCAL_REG_PORT_B = 0x00;

void MCP23017_readReg(uint8_t reg)
{
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;

  //uint8_t i2c_txBuffer[1] = {reg};
  //uint8_t i2c_txBufferSize = sizeof(i2c_txBuffer);
  uint8_t i2c_rxBuffer[1] = {0};
  //uint8_t i2c_rxBufferIndex;

  // Initializing I2C transfer
  i2cTransfer.addr          = MCP23017_ADDRESS;
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
  emberAfCorePrintln("err %d\n", result);
  emberAfCorePrintln("MCP23017_readReg.... %02X  Done   %02X\n",reg, i2c_rxBuffer[0]);
}


void MCP23017_writeReg(uint8_t reg, uint8_t value)
{
  interface_iic_write(MCP23017_ADDRESS, reg, &value, 1);
}

void MCP23017_GPIO_PinOutToggle(uint8_t port, uint8_t pin)
{
  uint8_t mask = 1 << pin;
  if(port == MCP23017_GPIO_A)
  {
    LOCAL_REG_PORT_A = (uint8_t)(LOCAL_REG_PORT_A ^ mask);
    MCP23017_writeReg(MCP23017_GPIO_A,LOCAL_REG_PORT_A);
  }
  if(port == MCP23017_GPIO_B)
  {
    LOCAL_REG_PORT_B = (uint8_t)(LOCAL_REG_PORT_B ^ mask);
    MCP23017_writeReg(MCP23017_GPIO_B,LOCAL_REG_PORT_B);
  }
}
void MCP23017_GPIO_PinOutSet(uint8_t port, uint8_t pin, uint8_t value)
{
  uint8_t mask = 1 << pin;
  if(port == MCP23017_GPIO_A)
  {
    LOCAL_REG_PORT_A = value?(uint8_t)(LOCAL_REG_PORT_A | mask):(uint8_t)(LOCAL_REG_PORT_A & ~mask);
    MCP23017_writeReg(MCP23017_GPIO_A,LOCAL_REG_PORT_A);
  }
  if(port == MCP23017_GPIO_B)
  {
    LOCAL_REG_PORT_B = value?(uint8_t)(LOCAL_REG_PORT_B | mask):(uint8_t)(LOCAL_REG_PORT_B & ~mask);
    MCP23017_writeReg(MCP23017_GPIO_B,LOCAL_REG_PORT_B);
  }
}


void MCP23017_Init()
{
  emberAfCorePrintln("MCP23017_Init......");
  //  disable address increment (datasheet)
  //MCP23017_writeReg(MCP23017_IOCR, 0b00100000);
  //  Force INPUT_PULLUP
  MCP23017_writeReg(MCP23017_PUR_A, 0xFF);
  MCP23017_writeReg(MCP23017_PUR_B, 0xFF);

  //  Ports Direction bit 0-Output,  bit 1-Input
  MCP23017_writeReg(MCP23017_DDR_A, 0x00);
  MCP23017_writeReg(MCP23017_DDR_B, 0x00);

  LOCAL_REG_PORT_A = 0x00;
  LOCAL_REG_PORT_B = 0x00;

  MCP23017_writeReg(MCP23017_GPIO_A,LOCAL_REG_PORT_A);
  MCP23017_writeReg(MCP23017_GPIO_B,LOCAL_REG_PORT_B);

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

  emberAfCorePrintln("MCP23017_Init......Done !!!");
}
