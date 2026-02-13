/*
 * App_StatusUpdate.c
 *
 *  Created on: Aug 24, 2022
 *      Author: Confio
 */

    #include "App_Src/App_Main.h"
    #include "App_Src/App_StatusUpdate.h"

    #define DESTINATION_STORTID 0x0000
    #define EVENT_BUTTON_PRESS  0x01

    EmberEventControl app_Endpoint1StatusUpdateEventControl;
    EmberEventControl app_Endpoint2StatusUpdateEventControl;
    EmberEventControl app_Endpoint3StatusUpdateEventControl;
    EmberEventControl app_Endpoint4StatusUpdateEventControl;
    EmberEventControl app_Endpoint5StatusUpdateEventControl;
    EmberEventControl app_Endpoint6StatusUpdateEventControl;
    EmberEventControl appAnnouncementEventControl;


    uint8_t Bat_Percent[1];

    int Get_Randoms()
      {
        int num = (halCommonGetRandom() %
        (30000 - (15000) + 1)) + 15000;

        return num;
      }



    void appAnnouncementEventHandler()
    {
      emberEventControlSetInactive(appAnnouncementEventControl);
      uint8_t Radio_channel = emberAfGetRadioChannel();

      if(Broadcast_seq>0xFA)
        {
          Broadcast_seq =1;
        }

      EmberApsFrame emAfCommandApsFrame;
      emAfCommandApsFrame.profileId = 0xC25D;
      emAfCommandApsFrame.clusterId = 0x0001;
      emAfCommandApsFrame.sourceEndpoint = 0xC4;
      emAfCommandApsFrame.destinationEndpoint = 0xC4;
      emAfCommandApsFrame.options = EMBER_APS_OPTION_SOURCE_EUI64 | \
                                    EMBER_APS_OPTION_DESTINATION_EUI64;

      uint8_t data[] = {0x18, //APS frame control
                        Broadcast_seq, //Sequence
                        0x0A, //cmd(Report attributes)

                        0x00, 0x00,
                        0x20,
                        0x03,

                        0x04, 0x00, //FIRMWARE_VERSION cmd
                        0x42, // type (Char String)
                        0x08, // length
                        0x30, MAIN_FW_VER, 0x2E, 0x30, MAJOR_BUG_FIX, 0x2E, 0x30, MINOR_BUG_FIX,    // 01.00.00

                        0x05, 0x00, //REFLASH_VERSION
                        0x20, // type Unsigned 8-bit integer
                        0x0FF,

                        0x06, 0x00,//BOOT_COUNT
                        0x21, // type Unsigned 16-bit integer
                        Boot_Count&0x00FF,Boot_Count>>8,

                        0x07, 0x00, //PRODUCT_STRING
                        0x42, // type (Char String)
//                        0x0B, //Length
//                        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x34, 0x52, //(Confio:CT4R)
//                        0x0F, //Length
//                        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x4D, 0x54, 0x53, 0x53, 0x43, 0x4E, //(Confio:CTMTSSCN)

                        0x12,                       // Length
                        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x52, 0x53, 0x34, 0x38, 0x35, 0x52, 0x45, 0x4D, 0x4F, 0x54, 0x45, //(Confio:RS485REMOTE)
                        0x0c, 0x00,  //MESH_CHANNEL
                        0x20,
                        Radio_channel,
                        };

       uint16_t dataLength = sizeof(data);
       EmberStatus status = emberAfSendUnicastWithCallback(EMBER_OUTGOING_DIRECT,
                                                           0x0000,
                                                           &emAfCommandApsFrame,
                                                           dataLength,
                                                           data,
                                                           NULL);

       Broadcast_seq++;

       emberEventControlSetDelayMS(appAnnouncementEventControl, (300000+Get_Randoms()));
       emberAfCorePrintln("======Announcement :  0x%02X\n",status);
    }






    void app_Endpoint1StatusUpdateEventHandler()
    {
      emberEventControlSetInactive(app_Endpoint1StatusUpdateEventControl);
      uint8_t buffer[5];
      EmberAfClusterId clusterId ;
      uint16_t reportAttributeRecordsLen =  5;
      if (Battery_Per_Identifier == true)
        {
          clusterId = 0x001;
          uint8_t buffer[13] =
            { 0x21, 0x00,     // Attribute_ID
                0x20,           // data type
                Battery_Percent, MACID[0], MACID[1],
                MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7], Endpoint_1 };
          emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                                     (uint8_t *) buffer,
                                                                     reportAttributeRecordsLen);
          Battery_Per_Identifier = false;
        }
      else
        {
          clusterId = 0xfc00;
          uint8_t buffer[5] =
            { 0x00, 0xc4,     // Attribute_ID
                0x21,           // data type
                EVENT_BUTTON_PRESS, Endpoint_1 };
          emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                                     (uint8_t *) buffer,
                                                                     reportAttributeRecordsLen);
        }






     emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
     emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
     EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                     DESTINATION_STORTID);
      emberAfCorePrintln("=========Ep1  :  0x%02X\n",status1);

    }


    void app_Endpoint2StatusUpdateEventHandler()
    {
      emberEventControlSetInactive(app_Endpoint2StatusUpdateEventControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[5] = {0x00, 0xc4,     // Attribute_ID
                           0x21,           // data type
                           EVENT_BUTTON_PRESS, MACID[0], MACID[1],
                           MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7], Endpoint_2};

      uint16_t reportAttributeRecordsLen =  5;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("=========Ep2  :  0x%02X\n",status1);


    }


    void app_Endpoint3StatusUpdateEventHandler()
    {
      emberEventControlSetInactive(app_Endpoint3StatusUpdateEventControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[5] = {0x00, 0xc4,     // Attribute_ID
                           0x21,           // data type
                           EVENT_BUTTON_PRESS, MACID[0], MACID[1],
                           MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7], Endpoint_3};

      uint16_t reportAttributeRecordsLen =  5;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);


      emberAfGetCommandApsFrame()->sourceEndpoint =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("=========Ep3  :  0x%02X\n",status1);
    }


    void app_Endpoint4StatusUpdateEventHandler()
    {
      emberEventControlSetInactive(app_Endpoint4StatusUpdateEventControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[5] = {0x00, 0xc4,     // Attribute_ID
                           0x21,           // data type
                           EVENT_BUTTON_PRESS, MACID[0], MACID[1],
                           MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7], Endpoint_4};
      uint16_t reportAttributeRecordsLen =  5;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                            (uint8_t *) buffer,
                                                            reportAttributeRecordsLen);


      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                               DESTINATION_STORTID);
      emberAfCorePrintln("=========Ep4  :  0x%02X\n",status1);
    }


    void app_Endpoint5StatusUpdateEventHandler()
       {
         emberEventControlSetInactive(app_Endpoint5StatusUpdateEventControl);
         EmberAfClusterId clusterId = 0xfc00;

         uint8_t buffer[5] = {0x00, 0xc4,     // Attribute_ID
                              0x21,           // data type
                              EVENT_BUTTON_PRESS , MACID[0], MACID[1],
                              MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7],  Endpoint_5};
         uint16_t reportAttributeRecordsLen =  5;


         emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                               (uint8_t *) buffer,
                                                               reportAttributeRecordsLen);


         emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
         emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
         EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                  DESTINATION_STORTID);
         emberAfCorePrintln("=========Ep5  :  0x%02X\n",status1);
       }



    void app_Endpoint6StatusUpdateEventHandler()
       {
         emberEventControlSetInactive(app_Endpoint6StatusUpdateEventControl);
         EmberAfClusterId clusterId = 0xfc00;
         uint8_t buffer[5] = {0x00, 0xc4,     // Attribute_ID
                              0x21,           // data type
                              EVENT_BUTTON_PRESS , MACID[0], MACID[1],
                              MACID[2], MACID[3], MACID[4], MACID[5], MACID[6], MACID[7],  Endpoint_6};

         uint16_t reportAttributeRecordsLen =  5;


         emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                               (uint8_t *) buffer,
                                                               reportAttributeRecordsLen);


         emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
         emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
         EmberStatus status1 =  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                  DESTINATION_STORTID);
         emberAfCorePrintln("=========Ep6  :  0x%02X\n",status1);
       }

