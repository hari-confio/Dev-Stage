/*
 * App_LED.h
 *
 *  Created on: 12-Jul-2024
 *      Author: saisu
 */

#ifndef APP_SCR_APP_LED_H_
#define APP_SCR_APP_LED_H_

void Dim_Init();
void set_proximity_level();

void set_led_when_zigbee_follow_connection_event(uint8_t led, uint8_t state);
void set_led_when_zigbee_on_off_event(uint8_t led, uint8_t state);
void set_led_when_button_event(uint8_t button, uint8_t state);
void set_led(uint8_t led, uint8_t state);

/*
 * C4_Led
 * 0 - Just Toggle indication(Default)
 * 1 - Follow Connection
 * 2 - Follow Internal Relay 1
 * 3 - Follow Internal Relay 2
 * 4 - Follow Internal Relay 3
 */
extern uint8_t C4_Led_1;
extern uint8_t C4_Led_2;
extern uint8_t C4_Led_3;
extern uint8_t C4_Led_4;
extern uint8_t C4_Led_5;
extern uint8_t C4_Led_6;
extern uint8_t C4_Led_7;
extern uint8_t C4_Led_8;

/*
 * C4_Led_Dim_Up_Rate
 * 0      - Instant
 * 1-255  - 1-255 mSec
 * 10     - Default
 */
extern uint8_t C4_Led_Dim_Up_Rate;
/*
 * C4_Led_Dim_Down_Rate
 * 0      - Instant
 * 1-255  - 1-255 mSec
 * 10     - Default
 */
extern uint8_t C4_Led_Dim_Down_Rate;

/*
 * C4_Led_Dim_Min
 * 0       - Completely OFF
 * 1-50    - % of Brightness
 * 10      - Default
 */
extern uint8_t C4_Led_Dim_Min;

/*
 * C4_Led_Dim_Max
 * 100     - Completely ON
 * 50-100  - % of Brightness
 * 100     - Default
 */
extern uint8_t C4_Led_Dim_Max;

/*
 * C4_Led_On_Time
 * 1-255  - 1-255 Sec
 * 5      - Default
 */
extern uint8_t C4_Led_On_Time;

#endif /* APP_SCR_APP_LED_H_ */
