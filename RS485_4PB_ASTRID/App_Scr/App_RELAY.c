/*
 * App_RELAY.c
 *
 *  Created on: 12-Jul-2024
 *      Author: saisu
 */
#include "MCP23017_I2C.h"
#include "MCP23017_2_I2C.h"
#include "App_Scr/App_Main.h"
#include "App_StatusUpdate.h"
#include "App_RELAY.h"
#include "App_LED.h"
/*
 * C4_Button_Relay
 * 0 - Not assigned
 * 1 - Trigger internal Relay 1
 * 2 - Trigger internal Relay 2
 * 3 - Trigger internal Relay 3
 */
uint8_t C4_Button_Relay_1 = 0;
uint8_t C4_Button_Relay_2 = 0;
uint8_t C4_Button_Relay_3 = 0;
uint8_t C4_Button_Relay_4 = 0;
uint8_t C4_Button_Relay_5 = 0;
uint8_t C4_Button_Relay_6 = 0;
uint8_t C4_Button_Relay_7 = 0;
uint8_t C4_Button_Relay_8 = 0;

uint8_t Relay_1_State = 0;
uint8_t Relay_2_State = 0;
uint8_t Relay_3_State = 0;

static void relay_on(uint8_t relay)
{
  switch(relay)
  {
    case 1:
      Relay_1_State = 1;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_1_PORT, APP_RELAY_1_PIN, 1);
    break;

    case 2:
      Relay_2_State = 1;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_2_PORT, APP_RELAY_2_PIN, 1);
    break;

    case 3:
      Relay_3_State = 1;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_3_PORT, APP_RELAY_3_PIN, 1);
    break;
  }
}

static void relay_off(uint8_t relay)
{
  switch(relay)
  {
    case 1:
      Relay_1_State = 0;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_1_PORT, APP_RELAY_1_PIN, 0);
    break;

    case 2:
      Relay_2_State = 0;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_2_PORT, APP_RELAY_2_PIN, 0);
    break;

    case 3:
      Relay_3_State = 0;
      MCP23017_2_GPIO_PinOutSet(APP_RELAY_3_PORT, APP_RELAY_3_PIN, 0);
    break;
  }
}

static void toggle_relay(uint8_t relay)
{
  switch(relay)
  {
    case 1:
      if(Relay_1_State)
          relay_off(relay);
      else
          relay_on(relay);
    break;

    case 2:
      if(Relay_2_State)
          relay_off(relay);
      else
          relay_on(relay);
    break;

    case 3:
      if(Relay_3_State)
          relay_off(relay);
      else
          relay_on(relay);
    break;
  }
}

void set_relay(uint8_t relay, uint8_t onOff)
{
  if(onOff)
    relay_on(relay);
  else
    relay_off(relay);
}

void set_relay_when_zigbee_event(uint8_t relay, uint8_t onOff)
{
  set_proximity_level();
  if(onOff)
    relay_on(relay);
  else
    relay_off(relay);
}


/*
 * C4_Button_Relay = 0; Not assigned
 * C4_Button_Relay = x; Trigger internal Relay x (1-3)
 */
static void set_c4_connected_internal_relay(uint8_t C4_Button_Relay)
{
  switch(C4_Button_Relay)
  {
    case 0://Not assigned, Ignore..
      //Nothing to do ..
    break;

    case 1://Relay 1
      toggle_relay(1);
      set_led_when_zigbee_on_off_event(1,0);
      emberEventControlSetDelayMS(app_Relay1StatusWriteNvm3EventControl,500);
      //emberEventControlSetDelayMS(app_Relay1StatusUpdateEventControl,1000);
    break;

    case 2://Relay 2
      toggle_relay(2);
      set_led_when_zigbee_on_off_event(2,0);
      emberEventControlSetDelayMS(app_Relay2StatusWriteNvm3EventControl,500);
      //emberEventControlSetDelayMS(app_Relay2StatusUpdateEventControl,1000);
    break;

    case 3://Relay 3
      toggle_relay(3);
      set_led_when_zigbee_on_off_event(3,0);
      emberEventControlSetDelayMS(app_Relay3StatusWriteNvm3EventControl,500);
      //emberEventControlSetDelayMS(app_Relay3StatusUpdateEventControl,1000);
    break;
  }
}


static void set_relay_when_button_pressed(uint8_t button)
{
  //Nothing to do..
}

static void set_relay_when_button_relesed(uint8_t button)
{
  switch(button)
  {
    case 1:
      set_c4_connected_internal_relay(C4_Button_Relay_1);
    break;
    case 2:
      set_c4_connected_internal_relay(C4_Button_Relay_2);
    break;
    case 3:
      set_c4_connected_internal_relay(C4_Button_Relay_3);
    break;
    case 4:
      set_c4_connected_internal_relay(C4_Button_Relay_4);
    break;
    case 5:
      set_c4_connected_internal_relay(C4_Button_Relay_5);
    break;
    case 6:
      set_c4_connected_internal_relay(C4_Button_Relay_6);
    break;
    case 7:
      set_c4_connected_internal_relay(C4_Button_Relay_7);
    break;
    case 8:
      set_c4_connected_internal_relay(C4_Button_Relay_8);
    break;
  }
}


void set_relay_when_button_event(uint8_t button, uint8_t state)
{
  if(state == 0)
    set_relay_when_button_relesed(button);
  else
    set_relay_when_button_pressed(button);

}
