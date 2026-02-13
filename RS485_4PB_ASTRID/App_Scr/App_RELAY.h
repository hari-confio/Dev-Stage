/*
 * App_RELAY.h
 *
 *  Created on: 12-Jul-2024
 *      Author: saisu
 */

#ifndef APP_SCR_APP_RELAY_H_
#define APP_SCR_APP_RELAY_H_

extern uint8_t C4_Button_Relay_1;
extern uint8_t C4_Button_Relay_2;
extern uint8_t C4_Button_Relay_3;
extern uint8_t C4_Button_Relay_4;
extern uint8_t C4_Button_Relay_5;
extern uint8_t C4_Button_Relay_6;
extern uint8_t C4_Button_Relay_7;
extern uint8_t C4_Button_Relay_8;

extern uint8_t Relay_1_State;
extern uint8_t Relay_2_State;
extern uint8_t Relay_3_State;

void set_relay_when_zigbee_event(uint8_t relay, uint8_t onOff);
void set_relay_when_button_event(uint8_t button, uint8_t state);
void set_relay(uint8_t relay, uint8_t onOff);

#endif /* APP_SCR_APP_RELAY_H_ */
