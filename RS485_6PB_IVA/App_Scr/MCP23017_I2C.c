/*
 * MCP23017_I2C.c
 *
 *  Created on: 23-Nov-2023
 *      Author: Confio
 */


#include "MCP23017_I2C.h"
#include "em_i2c.h"
#include "em_timer.h"
#include "em_cmu.h"
#include <iic/driver_interface.h>
#include "apds9960/app_apds9960.h"
#include "App_Scr/App_Main.h"

EmberEventControl smoothDimmingEventControl;

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
//	emberAfCorePrintln("err %d\n", result);
//	emberAfCorePrintln("MCP23017_readReg.... %02X  Done   %02X\n",reg, i2c_rxBuffer[0]);
	if(reg == 0x12 && (i2c_rxBuffer[0] == 0xAA || i2c_rxBuffer[0] == 0x5A || i2c_rxBuffer[0] == 0x9A || i2c_rxBuffer[0] == 0x6A)) // SENSOR INTERRUPT
	  {
	    //emberAfCorePrintln("Sensor int\n");
	    set_proximity_level();
	  }
}


void MCP23017_writeReg(uint8_t reg, uint8_t value)
{
	apds9960_interface_iic_write(MCP23017_ADDRESS, reg, &value, 1);
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
  emberAfCorePrintln("-MCP23017_Init-");
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

	MCP23017_GPIO_PinOutSet(PORT_SENSOR_INT, PIN_SENSOR_INT, 1);

//	set_led(1,1);
//	set_led(2,1);
//	set_led(3,1);
//	set_led(4,1);
//	set_led(5,1);
//	set_led(6,1);

	//readNVM();
}
uint8_t dim_target_value = 0;
uint8_t dim_current_value = 0;
uint32_t timeCount = 0;

uint8_t C4_Led_Dim_Up_Rate    = 10;
uint8_t C4_Led_Dim_Down_Rate  = 10;
uint8_t C4_Led_Dim_Min        = 0;
uint8_t C4_Led_Dim_Max        = 100;
uint8_t C4_Led_On_Time        = 20;
uint32_t triac_compare_val1_1;

static void Set_Dim_levels(uint8_t Dim)
{
  uint32_t max = 0xBE6E;
  triac_compare_val1_1 = (max * (uint32_t)(Dim)) / 100;
  TIMER_CompareSet(TIMER0, 1, triac_compare_val1_1);
}

void smoothDimmingEventHandler()
{
  if(dim_target_value > dim_current_value)
  {
    emberEventControlSetDelayMS(smoothDimmingEventControl,C4_Led_Dim_Up_Rate);
    Set_Dim_levels(dim_current_value++);
    timeCount = 0;
  }
  else if(dim_target_value < dim_current_value)
  {
    emberEventControlSetDelayMS(smoothDimmingEventControl,C4_Led_Dim_Down_Rate);
    Set_Dim_levels(dim_current_value--);
    timeCount = 0;
  }
  else
  {
    emberEventControlSetDelayMS(smoothDimmingEventControl,10);//Hold time check for every 10mSec
    timeCount++;
    /*
     * We will come here every 10mSec
     * Total delay = (C4_Led_On_Time*100) * 10
     * Total delay = 5(sec) * 100 * 10
     * 500 Counts on every 10 mSec = 5Sec
     */
    if(timeCount == C4_Led_On_Time*100)
    {
        dim_target_value = C4_Led_Dim_Min;
       // emberAfCorePrintln("Led Dim Current %d, Target %d, Down Rate %d",dim_current_value,dim_target_value,C4_Led_Dim_Down_Rate);
    }

    if(dim_current_value == C4_Led_Dim_Min)
    {
       // emberAfCorePrintln("Led Dim Stop %d",C4_Led_On_Time);
        emberEventControlSetInactive(smoothDimmingEventControl);
        Set_Dim_levels(dim_current_value);
    }
  }
}

void set_proximity_level()
{
  emberEventControlSetDelayMS(smoothDimmingEventControl,C4_Led_Dim_Up_Rate);
  dim_target_value = C4_Led_Dim_Max;
  timeCount = 0;

 // emberAfCorePrintln("Led Dim Current %d, Target %d, Up Rate %d",dim_current_value,dim_target_value,C4_Led_Dim_Up_Rate);
}

void Dim_Init()
{
  emberAfCorePrintln("-dim-init-");
  CMU_ClockEnable(cmuClock_TIMER0, false);
  TIMER_Enable(TIMER0, false);

  GPIO_PinModeSet(DIM_PORT, DIM_PIN, gpioModeWiredAndPullUpFilter, ON);

  // Enable clock for TIMER0 module
  CMU_ClockEnable(cmuClock_TIMER0, true);

  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode = timerCCModePWM;

  TIMER_InitCC(TIMER0, 1, &timerCCInit);  //output triac 1

  //Set First compare value fot Triac 1
  uint32_t max = 0xBE6E;//800Hz
  triac_compare_val1_1 = (max * (uint32_t)(5)) / 100;
  TIMER_CompareSet(TIMER0, 1, triac_compare_val1_1);

  //Set Top value for Timer (should be less then 20mSec)
  max = 0xBE6E;//800Hz
  TIMER_TopSet(TIMER0, max);

  // Route CC0 output to PA6
  GPIO->TIMERROUTE[0].ROUTEEN  = GPIO_TIMER_ROUTEEN_CC1PEN;
  GPIO->TIMERROUTE[0].CC1ROUTE = (DIM_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
                      | (DIM_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);

  // Initialize and start timer
  TIMER_Init_TypeDef timerInit_TIM_LEC = TIMER_INIT_DEFAULT;
  timerInit_TIM_LEC.prescale = timerPrescale1;

  TIMER_Init(TIMER0, &timerInit_TIM_LEC);
  TIMER_Enable(TIMER0, true);

  set_proximity_level();
}

//void initialLeds()
