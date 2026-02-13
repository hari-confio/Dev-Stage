/*
 * App_LED.c
 *
 *  Created on: 12-Jul-2024
 *      Author: saisu
 */

#include "MCP23017_I2C.h"
#include "MCP23017_2_I2C.h"
#include "em_timer.h"
#include "App_Scr/App_Main.h"
#include "App_RELAY.h"

EmberEventControl smoothDimmingEventControl;

uint8_t dim_target_value = 0;
uint8_t dim_current_value = 0;
uint32_t timeCount = 0;

uint32_t triac_compare_val1_1;


uint8_t C4_Led_1 = 0;
uint8_t C4_Led_2 = 0;
uint8_t C4_Led_3 = 0;
uint8_t C4_Led_4 = 0;
uint8_t C4_Led_5 = 0;
uint8_t C4_Led_6 = 0;
uint8_t C4_Led_7 = 0;
uint8_t C4_Led_8 = 0;


uint8_t C4_Led_Dim_Up_Rate    = 10;
uint8_t C4_Led_Dim_Down_Rate  = 10;
uint8_t C4_Led_Dim_Min        = 0;
uint8_t C4_Led_Dim_Max        = 100;
uint8_t C4_Led_On_Time        = 20;

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
        emberAfCorePrintln("Led Dim Stop %d",C4_Led_On_Time);
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

  emberAfCorePrintln("Led Dim Current %d, Target %d, Up Rate %d",dim_current_value,dim_target_value,C4_Led_Dim_Up_Rate);
}


void Dim_Init()
{
  emberAfCorePrintln("--dim-init--");
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






static void led_off(uint8_t led)
{
  switch(led)
  {
    case 1:
      emberAfCorePrintln("\n====led_off button==1==\n");

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
    break;
    case 2:
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
    break;
    case 3:
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
    break;
    case 4:
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
    break;
    case 5:
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
    break;
    case 6:
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
    break;
    case 7:
      //emberAfCorePrintln("\n====led_off button==7==\n");

      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
    break;
    case 8:
     // emberAfCorePrintln("\n====led_off button==8==\n");

      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
    break;
  }
}

static void led_on(uint8_t led)
{
  switch(led)
  {
    case 1:
      emberAfCorePrintln("\n====led_on button==1==\n");
      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 1);
    break;
    case 2:
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 1);
    break;
    case 3:
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 1);
    break;
    case 4:
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 1);
    break;
    case 5:
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 1);
    break;
    case 6:
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 1);
    break;
    case 7:
     // emberAfCorePrintln("\n====led_on button==7==\n");
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 1);
    break;
    case 8:
     // emberAfCorePrintln("\n====led_on button==8==\n");
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 1);
    break;
  }
}
extern int btnSceneLed;

void set_led(uint8_t led, uint8_t state)
{
  set_proximity_level();
  if(state)
    led_on(led);
  else
    led_off(led);

  if(btnSceneLed == 1)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
    }
  if(btnSceneLed == 2)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
    }
  if(btnSceneLed == 3)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
       MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
    }
  if(btnSceneLed == 4)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
    }
  if(btnSceneLed == 5)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
    }
  if(btnSceneLed == 6)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
    }
  if(btnSceneLed == 7)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
    }
  if(btnSceneLed == 8)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
    }

}

void set_btnSceneLed(uint8_t btnSceneLed)
{
  set_proximity_level();
  if(btnSceneLed == 1)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
     //  writeNVM(1);
    }
  if(btnSceneLed == 2)
    {
      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
    //  writeNVM(2);
    }
  if(btnSceneLed == 3)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
     // writeNVM(3);
    }
  if(btnSceneLed == 4)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
     // writeNVM(4);
    }
  if(btnSceneLed == 5)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
    //  writeNVM(5);
    }
  if(btnSceneLed == 6)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
     // writeNVM(6);
    }
  if(btnSceneLed == 7)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 0);
     // writeNVM(6);
    }
  if(btnSceneLed == 8)
    {

      MCP23017_GPIO_PinOutSet(APP_LED_1_ON_PORT, APP_LED_1_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_1_OFF_PORT, APP_LED_1_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_2_ON_PORT, APP_LED_2_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_2_OFF_PORT, APP_LED_2_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_3_ON_PORT, APP_LED_3_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_3_OFF_PORT, APP_LED_3_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_4_ON_PORT, APP_LED_4_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_4_OFF_PORT, APP_LED_4_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_5_ON_PORT, APP_LED_5_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_5_OFF_PORT, APP_LED_5_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_6_ON_PORT, APP_LED_6_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_6_OFF_PORT, APP_LED_6_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_7_ON_PORT, APP_LED_7_ON_PIN, 1);
      MCP23017_GPIO_PinOutSet(APP_LED_7_OFF_PORT, APP_LED_7_OFF_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_ON_PORT, APP_LED_8_ON_PIN, 0);
      MCP23017_GPIO_PinOutSet(APP_LED_8_OFF_PORT, APP_LED_8_OFF_PIN, 1);
     // writeNVM(6);
    }

}


static void check_led_c4_param_when_button_event(uint8_t C4_Led, uint8_t button, uint8_t state)
{
  if(C4_Led == 0)//Just Toggle indication
  {
      set_led(button, state);
  }
}


void set_led_when_button_event(uint8_t button, uint8_t state)
{
  set_proximity_level();
  switch(button)
  {
    case 1:
      C4_Led_1 = 0;
      check_led_c4_param_when_button_event(C4_Led_1,button,state);
    break;
    case 2:
      check_led_c4_param_when_button_event(C4_Led_2,button,state);
    break;
    case 3:
      check_led_c4_param_when_button_event(C4_Led_3,button,state);
    break;
    case 4:
      check_led_c4_param_when_button_event(C4_Led_4,button,state);
    break;
    case 5:
      check_led_c4_param_when_button_event(C4_Led_5,button,state);
    break;
    case 6:
      check_led_c4_param_when_button_event(C4_Led_6,button,state);
    break;
    case 7:
      check_led_c4_param_when_button_event(C4_Led_7,button,state);
    break;
    case 8:
      //C4_Led_8 = 0;
      check_led_c4_param_when_button_event(C4_Led_8,button,state);
    break;
  }
}










static void check_led_c4_param_when_zigbee_on_off_event(uint8_t C4_Led, uint8_t relay, uint8_t led)
{
  if(C4_Led == 0)//Just Toggle indication
  {
      //Nothing to do here..
  }
  if(C4_Led == 1)//Follow Connection
  {
      //Nothing to do here..
  }
  if(C4_Led == 2 && relay == 1)//Follow Internal Relay 1
  {
      set_led(led,Relay_1_State);
  }
  if(C4_Led == 3 && relay == 2)//Follow Internal Relay 2
  {
      set_led(led,Relay_2_State);
  }
  if(C4_Led == 4 && relay == 3)//Follow Internal Relay 3
  {
      set_led(led,Relay_3_State);
  }
}


void set_led_when_zigbee_on_off_event(uint8_t led, uint8_t state)
{
  set_proximity_level();
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_1,led,1);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_2,led,2);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_3,led,3);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_4,led,4);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_5,led,5);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_6,led,6);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_7,led,7);
  check_led_c4_param_when_zigbee_on_off_event(C4_Led_8,led,8);
}







static void check_led_c4_param_when_zigbee_follow_connection_event(uint8_t C4_Led, uint8_t button, uint8_t state)
{
  if(C4_Led == 0)//Just Toggle indication
  {
      //Nothing to do here..
  }
  if(C4_Led == 1)//Follow Connection
  {
      set_led(button,state);
  }
  if(C4_Led == 2)//Follow Internal Relay 1
  {
      //Nothing to do here..
  }
  if(C4_Led == 3)//Follow Internal Relay 2
  {
      //Nothing to do here..
  }
  if(C4_Led == 4)//Follow Internal Relay 3
  {
      //Nothing to do here..
  }
}


void set_led_when_zigbee_follow_connection_event(uint8_t led, uint8_t state)
{
  set_proximity_level();
  switch(led)
  {
    case 1:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_1,led,state);
    break;
    case 2:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_2,led,state);
    break;
    case 3:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_3,led,state);
    break;
    case 4:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_4,led,state);
    break;
    case 5:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_5,led,state);
    break;
    case 6:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_6,led,state);
    break;
    case 7:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_7,led,state);
    break;
    case 8:
      check_led_c4_param_when_zigbee_follow_connection_event(C4_Led_8,led,state);
    break;
  }
}
