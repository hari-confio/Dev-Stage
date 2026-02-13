/*
 * App_StatusUpdate.h
 *
 *  Created on: 13-Jan-2023
 *      Author: Confio
 */

#ifndef APP_SCR_APP_STATUSUPDATE_H_
#define APP_SCR_APP_STATUSUPDATE_H_

extern EmberEventControl app_Endpoint1StatusUpdateEventControl;
extern EmberEventControl app_Endpoint2StatusUpdateEventControl;
extern EmberEventControl app_Endpoint3StatusUpdateEventControl;
extern EmberEventControl app_Endpoint4StatusUpdateEventControl;
extern EmberEventControl app_Endpoint5StatusUpdateEventControl;
extern EmberEventControl app_Endpoint6StatusUpdateEventControl;
extern EmberEventControl app4RAnnouncementEventControl;
extern EmberEventControl app_Relay1StatusUpdateEventControl;
extern EmberEventControl app_Relay2StatusUpdateEventControl;
extern EmberEventControl app_Relay3StatusUpdateEventControl;
extern EmberEventControl app_Relay1StatusWriteNvm3EventControl;
extern EmberEventControl app_Relay2StatusWriteNvm3EventControl;
extern EmberEventControl app_Relay3StatusWriteNvm3EventControl;


void set_button_event(uint8_t active_button, uint8_t active_count);
void send_zigbee_button_event(uint8_t active_button, uint8_t active_count);

extern bool Network_Steer;

#endif /* APP_SRC_APP_STATUSUPDATE_H_ */

