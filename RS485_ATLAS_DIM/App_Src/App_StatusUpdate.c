/*
 * App_StatusUpdate.c
 *
 *  Created on: 31-Jan-2023
 *      Author: Confio
 */

 #include "App_Src/App_Main.h"
 #include "App_Src/App_StatusUpdate.h"

#define DESTINATION_STORTID     0x0000


EmberEventControl app3B1RAnnouncementEventControl;
EmberEventControl app_Button_1_StatusUpdateEventControl;
EmberEventControl app_Button_2_StatusUpdateEventControl;
EmberEventControl app_Button_3_StatusUpdateEventControl;
EmberEventControl app_Button_Rotatory_StatusUpdateEventControl;
EmberEventControl app_Button_Rotatory_UpdateEventControl;

int Get_Randoms()
      {
        int num = (halCommonGetRandom() %
        (30000 - (15000) + 1)) + 15000;

        return num;
      }

void app3B1RAnnouncementEventHandler()
{
  emberEventControlSetInactive(app3B1RAnnouncementEventControl);

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
          0x0C,                      // Length
          0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x41, 0x54, 0x4C, 0x41, 0x53,    //(Confio:ATLAS)

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

  emberEventControlSetDelayMS(app3B1RAnnouncementEventControl, (300000+Get_Randoms()));
}

/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 1
/////////////////////////////////////////////////////////////////////////////////

void app_Button_1_StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Button_1_StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,                 // Attrubute_ID
                       0x21,                       // data type(unsigned 8 bit)
                       BUTTON_STATUS, 1};

  uint16_t reportAttributeRecordLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                        (uint8_t *) buffer,
                                                        reportAttributeRecordLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = Endpoint_1;
    emberAfGetCommandApsFrame() -> destinationEndpoint = Endpoint_1;
    EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                    DESTINATION_STORTID);
    emberAfCorePrintln("===========Ep1   : 0x%02X\n",status1);
}

/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 2
/////////////////////////////////////////////////////////////////////////////////
void app_Button_2_StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Button_2_StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,                 // Attrubute_ID
                       0x21,                       // data type(unsigned 8 bit)
                       BUTTON_STATUS, 2};

  uint16_t reportAttributeRecordLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                        (uint8_t *) buffer,
                                                        reportAttributeRecordLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = Endpoint_1;
    emberAfGetCommandApsFrame() -> destinationEndpoint = Endpoint_1;
    EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                    DESTINATION_STORTID);
    emberAfCorePrintln("===========Ep2  : 0x%02X\n",status1);
}

/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Switch 3
/////////////////////////////////////////////////////////////////////////////////
void app_Button_3_StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Button_3_StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,                 // Attrubute_ID
                       0x21,                       // data type(unsigned 8 bit)
                       BUTTON_STATUS, 3};

  uint16_t reportAttributeRecordLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                        (uint8_t *) buffer,
                                                        reportAttributeRecordLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = Endpoint_1;
    emberAfGetCommandApsFrame() -> destinationEndpoint = Endpoint_1;
    EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                    DESTINATION_STORTID);
    emberAfCorePrintln("===========Ep3   : 0x%02X\n",status1);
}

/////////////////////////////////////////////////////////////////////////////////
/// Sending the data from Rotatory Switch
/////////////////////////////////////////////////////////////////////////////////
void app_Button_Rotatory_StatusUpdateEventHandler()
{
  emberEventControlSetInactive(app_Button_Rotatory_StatusUpdateEventControl);

  EmberAfClusterId clusterId = 0xfc00;

  uint8_t buffer[5] = {0x00, 0xc4,                 // Attrubute_ID
                       0x21,                       // data type(unsigned 8 bit)
                       BUTTON_STATUS, 4};

  uint16_t reportAttributeRecordLen = 5;

  emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                        (uint8_t *) buffer,
                                                        reportAttributeRecordLen);

  emberAfGetCommandApsFrame() -> sourceEndpoint = Endpoint_1;
    emberAfGetCommandApsFrame() -> destinationEndpoint = Endpoint_1;
    EmberStatus status1 = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                    DESTINATION_STORTID);
    emberAfCorePrintln("===========Ep4   : 0x%02X\n",status1);
}



