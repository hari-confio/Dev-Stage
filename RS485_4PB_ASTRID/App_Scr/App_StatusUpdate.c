/*
 * App_StatusUpdate.c
 *
 *  Created on: 13-Jan-2023
 *      Author: Confio
 */

#include "App_Scr/App_Main.h"
#include "App_Scr/App_StatusUpdate.h"
#include "App_RELAY.h"

#define DESTINATION_STORTID     0x0000
#define EVENT_BUTTON_PRESS      0x01   // 0x01 is the button press command in the from document.

EmberEventControl app_Endpoint1StatusUpdateEventControl;
EmberEventControl app_Endpoint2StatusUpdateEventControl;
EmberEventControl app_Endpoint3StatusUpdateEventControl;
EmberEventControl app_Endpoint4StatusUpdateEventControl;
EmberEventControl app_Endpoint5StatusUpdateEventControl;
EmberEventControl app_Endpoint6StatusUpdateEventControl;
EmberEventControl app_Endpoint7StatusUpdateEventControl;
EmberEventControl app_Endpoint8StatusUpdateEventControl;
EmberEventControl app4RAnnouncementEventControl;
EmberEventControl app_Relay1StatusUpdateEventControl;
EmberEventControl app_Relay2StatusUpdateEventControl;
EmberEventControl app_Relay3StatusUpdateEventControl;
EmberEventControl app_Relay1StatusWriteNvm3EventControl;
EmberEventControl app_Relay2StatusWriteNvm3EventControl;
EmberEventControl app_Relay3StatusWriteNvm3EventControl;

uint8_t button_1_count = 0;
uint8_t button_2_count = 0;
uint8_t button_3_count = 0;
uint8_t button_4_count = 0;
uint8_t button_5_count = 0;
uint8_t button_6_count = 0;
uint8_t button_7_count = 0;
uint8_t button_8_count = 0;



int Get_Randoms()
      {
        int num = (halCommonGetRandom() %
        (30000 - (15000) + 1)) + 15000;

        return num;
      }

void app4RAnnouncementEventHandler()
{
  emberEventControlSetInactive(app4RAnnouncementEventControl);

  uint8_t Radio_channel = emberAfGetRadioChannel ();

  if(Broadcast_seq > 0xFA)
    {
      Broadcast_seq = 1;
    }
  EmberApsFrame emAfCommandApsFrame;
  emAfCommandApsFrame.profileId = 0xC25D;
  emAfCommandApsFrame.clusterId = 0x0001;
  emAfCommandApsFrame.sourceEndpoint = 0xC4;
  emAfCommandApsFrame.destinationEndpoint = 0xC4;
  emAfCommandApsFrame.options = EMBER_APS_OPTION_SOURCE_EUI64 | \
                                EMBER_APS_OPTION_DESTINATION_EUI64;

  uint8_t data[] =
      {
          0x18,                       //APS frame control
          Broadcast_seq,              // Sequence
          0x0A,                       // cmd(Report attributes)

          0x00, 0x00,
          0x20,
          0x03,

          0x04, 0x00,                 // FIRMWARE_VIRSION cmd
          0x42,                       // type (char string)
          0x08,                       // length (8 bit)
          0x30, MAIN_FW_VER, 0x2E, 0x30, MAJOR_BUG_FIX, 0x2E, 0x30, MINOR_BUG_FIX, // 01.00.00

          0x05, 0x00,                 // REFLASH_VERSION
          0x20,                       // type Unsigned 8-bit integer
          0x0FF,

          0x06, 0x00,                 // BOOT_COUNT
          0x21,                       // type Unsigned 16-bit integer
          Boot_Count&0x00FF,Boot_Count>>8,

          0x07, 0x00,                 // PRODUCT_STRING
          0x42,                       // type (Char String)

          //0x0B,                       // Length
          //0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x34, 0x52, //(Confio:CT4R)

          //0x0E, //Length
          // 0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x4D, 0x54, 0x53, 0x34, 0x52, //(Confio:CTMTS4R)

          0x0C,                       // Length
          0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x38, 0x50, 0x42, //(Confio:CT8PB)

          0x0c, 0x00,  //MESH_CHANNEL
          0x20,
          Radio_channel, };

  uint16_t dataLength = sizeof(data);
  EmberStatus status = emberAfSendUnicastWithCallback(EMBER_OUTGOING_DIRECT,
                                                             0x0000,
                                                             &emAfCommandApsFrame,
                                                             dataLength,
                                                             data,
                                                             NULL);

  Broadcast_seq++;

  emberEventControlSetDelayMS(app4RAnnouncementEventControl, (300000+Get_Randoms()));
}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 1
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint1StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint1StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,              // Attribute_ID
                       0x21,                    // data type
                       button_1_count, 1};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);
  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);
 // emberAfCorePrintln("===========Ep1   : 0x%02X",status1);


}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 2
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint2StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint2StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,            // Attribute_ID
                       0x21,                  // data type
                       button_2_count, 2};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast (EMBER_OUTGOING_DIRECT,
                                                   DESTINATION_STORTID);
  //emberAfCorePrintln("===========Ep2   : 0x%02X",status1);


}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 3
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint3StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint3StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,            // Attribute_ID
                       0x21,                  // data type
                       button_3_count,  3};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);
 // emberAfCorePrintln("===========Ep3   : 0x%02X",status1);

}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 4
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint4StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint4StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,            // Attribute_ID
                       0x21,                  // data type
                       button_4_count,   4};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);

 // emberAfCorePrintln("===========Ep4   : 0x%02X",status1);

}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 5
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint5StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint5StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,           // Attribute_ID
                       0x21,                // data type
                       button_5_count,  5};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);

  //emberAfCorePrintln("===========Ep5   : 0x%02X",status1);

}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 6
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint6StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint6StatusUpdateEventControl);

  EmberAfClusterId clusterId  = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,          // Attribute_ID
                       0x21,                // data type
                       button_6_count, 6};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);

 // emberAfCorePrintln("===========Ep6   : 0x%02X",status1);

}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 7
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint7StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint7StatusUpdateEventControl);

  EmberAfClusterId clusterId  = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,          // Attribute_ID
                       0x21,                // data type
                       button_7_count, 7};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);

  //emberAfCorePrintln("===========Ep7   : 0x%02X",status1);

}


/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 8
/////////////////////////////////////////////////////////////////////////////////
void app_Endpoint8StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Endpoint8StatusUpdateEventControl);

  EmberAfClusterId clusterId  = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,          // Attribute_ID
                       0x21,                // data type
                       button_8_count, 8};

  uint16_t reportAttributeRecordsLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *) buffer,
                                                         reportAttributeRecordsLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_C4;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_C4;
  EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);

 // emberAfCorePrintln("===========Ep8   : 0x%02X",status1);

}


void app_Relay1StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Relay1StatusUpdateEventControl);
  EmberAfClusterId clusterId = ZCL_ON_OFF_CLUSTER_ID;
  uint8_t buffer[4] =  {0x00, 0x00,              // Attribute_ID
                        ZCL_BOOLEAN_ATTRIBUTE_TYPE,                    // data type
                        Relay_1_State?1:0};
  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *)buffer,
                                                         4);
  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_1;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_1;
  EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);
  //emberAfCorePrintln("=Relay 1 Status Update : %d 0x%02X",Relay_1_State, status);
}

void app_Relay2StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Relay2StatusUpdateEventControl);
  EmberAfClusterId clusterId = ZCL_ON_OFF_CLUSTER_ID;
  uint8_t buffer[4] =  {0x00, 0x00,              // Attribute_ID
                        ZCL_BOOLEAN_ATTRIBUTE_TYPE,                    // data type
                        Relay_2_State?1:0};
  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *)buffer,
                                                         4);
  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_2;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_2;
  EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);
  //emberAfCorePrintln("=Relay 2 Status Update : %d 0x%02X",Relay_2_State, status);
}

void app_Relay3StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Relay3StatusUpdateEventControl);
  EmberAfClusterId clusterId = ZCL_ON_OFF_CLUSTER_ID;
  uint8_t buffer[4] =  {0x00, 0x00,              // Attribute_ID
                        ZCL_BOOLEAN_ATTRIBUTE_TYPE,                    // data type
                        Relay_3_State?1:0};
  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                         (uint8_t *)buffer,
                                                         4);
  emberAfGetCommandApsFrame() -> sourceEndpoint = ENDPOINT_3;
  emberAfGetCommandApsFrame() -> destinationEndpoint = ENDPOINT_3;
  EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                  DESTINATION_STORTID);
  //emberAfCorePrintln("=Relay 3 Status Update : %d 0x%02X",Relay_3_State, status);
}


void app_Relay1StatusWriteNvm3EventEventHandler()
{
  emberEventControlSetInactive(app_Relay1StatusWriteNvm3EventControl);
  EmberStatus status = emberAfWriteServerAttribute(ENDPOINT_1,
                                                   ZCL_ON_OFF_CLUSTER_ID,
                                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                                   (uint8_t*)&Relay_1_State,
                                                   ZCL_INT8U_ATTRIBUTE_TYPE);
 // emberAfCorePrintln("=Write 1 Status Update : %d 0x%02X",Relay_1_State, status);
}

void app_Relay2StatusWriteNvm3EventEventHandler()
{
  emberEventControlSetInactive(app_Relay2StatusWriteNvm3EventControl);
  EmberStatus status = emberAfWriteServerAttribute(ENDPOINT_2,
                                                   ZCL_ON_OFF_CLUSTER_ID,
                                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                                   (uint8_t*)&Relay_2_State,
                                                   ZCL_INT8U_ATTRIBUTE_TYPE);
  //emberAfCorePrintln("=Write 2 Status Update : %d 0x%02X",Relay_2_State, status);
}

void app_Relay3StatusWriteNvm3EventEventHandler()
{
  emberEventControlSetInactive(app_Relay3StatusWriteNvm3EventControl);
  EmberStatus status = emberAfWriteServerAttribute(ENDPOINT_3,
                                                   ZCL_ON_OFF_CLUSTER_ID,
                                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                                   (uint8_t*)&Relay_3_State,
                                                   ZCL_INT8U_ATTRIBUTE_TYPE);
  //emberAfCorePrintln("=Write 3 Status Update : %d 0x%02X",Relay_3_State, status);
}



void send_zigbee_button_event(uint8_t active_button, uint8_t active_count)
{
  if(active_button == 1)
      {
        button_1_count = active_count;
        emberEventControlSetActive(app_Endpoint1StatusUpdateEventControl);
      }

    if(active_button == 2)
      {
        button_2_count = active_count;
        emberEventControlSetActive(app_Endpoint2StatusUpdateEventControl);
      }

    if(active_button == 3)
      {
        button_3_count = active_count;
        emberEventControlSetActive(app_Endpoint3StatusUpdateEventControl);
      }

    if(active_button == 4)
      {
        button_4_count = active_count;
        emberEventControlSetActive(app_Endpoint4StatusUpdateEventControl);
      }

    if(active_button == 5)
      {
        button_5_count = active_count;
        emberEventControlSetActive(app_Endpoint5StatusUpdateEventControl);
      }

    if(active_button == 6)
      {
        button_6_count = active_count;
        emberEventControlSetActive(app_Endpoint6StatusUpdateEventControl);
      }
    if(active_button == 7)
      {
        button_7_count = active_count;
        emberEventControlSetActive(app_Endpoint7StatusUpdateEventControl);
      }
    if(active_button == 8)
      {
        button_8_count = active_count;
        emberEventControlSetActive(app_Endpoint8StatusUpdateEventControl);
      }
}
