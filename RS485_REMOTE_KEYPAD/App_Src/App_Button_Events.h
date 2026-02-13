/*
 * App_Button_Events.h
 *
 *  Created on: 22-Aug-2023
 *      Author: Confio
 */

#ifndef APP_SRC_APP_BUTTON_EVENTS_H_
#define APP_SRC_APP_BUTTON_EVENTS_H_

    #define EVENT_BUTTON1           0x01
    #define EVENT_BUTTON2           0x02
    #define EVENT_BUTTON3           0x03
    #define EVENT_BUTTON4           0x04
    #define EVENT_BUTTON5           0x05
    #define EVENT_BUTTON6           0x06

    #define EVENT_BUTTON_RELEASE    0x00
    #define EVENT_BUTTON_PRESS      0x01
    #define EVENT_BUTTON_CLICK      0x02
    #define EVENT_BUTTON_HOLD       0x03
    #define EVENT_MULTI_CLICK       0x80

    #define DESTINATION_STORTID 0x0000

    #define BUTTON_COUNT_RESET           0
    #define COUNTER_RESET                0
    #define TURN_OFF                     0
    #define TURN_ON                      1
    #define TOGGLE                       2

    int Button1_Event(int state);
    int Button2_Event(int state);
    int Button3_Event(int state);
    int Button4_Event(int state);
    int Button5_Event(int state);
    int Button6_Event(int state);

    extern EmberEventControl appButton1_EventDetectionReportingControl;
    extern EmberEventControl appButton2_EventDetectionReportingControl;
    extern EmberEventControl appButton3_EventDetectionReportingControl;
    extern EmberEventControl appButton4_EventDetectionReportingControl;
    extern EmberEventControl appButton5_EventDetectionReportingControl;
    extern EmberEventControl appButton6_EventDetectionReportingControl;

    void set_Button_event(uint8_t active_button, uint8_t active_count);

#endif /* APP_SRC_APP_BUTTON_EVENTS_H_ */
