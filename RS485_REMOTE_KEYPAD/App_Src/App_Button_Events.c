/*
 * App_Button_Events.c
 *
 *  Created on: 22-Aug-2023
 *      Author: Confio
 */

    #include "app/framework/include/af.h"
    #include "app/framework/util/util.h"
    #include "App_Main.h"
    #include "App_Button_Events.h"


/*********************************************************************************************************************************************************************
 **********************************************Non Latch Button Event Deection****************************************************************************************
 *********************************************************************************************************************************************************************/


    bool button1_417ms        = false;
    bool button1_1667ms       = false;
    uint8_t NonLatch1_click_count = 0;
    uint16_t button1_state     = 0x00;
    uint16_t Event_button1     = 0x00;

    bool button2_417ms        = false;
    bool button2_1667ms       = false;
    uint8_t NonLatch2_click_count = 0;
    uint16_t button2_state     = 0x00;
    uint16_t Event_button2     = 0x00;

    bool button3_417ms        = false;
    bool button3_1667ms       = false;
    uint8_t NonLatch3_click_count = 0;
    uint16_t button3_state     = 0x00;
    uint16_t Event_button3     = 0x00;

    bool button4_417ms        = false;
    bool button4_1667ms       = false;
    uint8_t NonLatch4_click_count = 0;
    uint16_t button4_state     = 0x00;
    uint16_t Event_button4     = 0x00;

    bool button5_417ms        = false;
    bool button5_1667ms       = false;
    uint8_t NonLatch5_click_count = 0;
    uint16_t button5_state     = 0x00;
    uint16_t Event_button5     = 0x00;

    bool button6_417ms        = false;
    bool button6_1667ms       = false;
    uint8_t NonLatch6_click_count = 0;
    uint16_t button6_state     = 0x00;
    uint16_t Event_button6     = 0x00;

    bool hold = false;  //to solve debounce
    EmberEventControl appButton1_417msEventControl;
    EmberEventControl appButton1_1667msEventControl;
    EmberEventControl appButton1_EventDetectionReportingControl;

    EmberEventControl appButton2_417msEventControl;
    EmberEventControl appButton2_1667msEventControl;
    EmberEventControl appButton2_EventDetectionReportingControl;

    EmberEventControl appButton3_417msEventControl;
    EmberEventControl appButton3_1667msEventControl;
    EmberEventControl appButton3_EventDetectionReportingControl;

    EmberEventControl appButton4_417msEventControl;
    EmberEventControl appButton4_1667msEventControl;
    EmberEventControl appButton4_EventDetectionReportingControl;

    EmberEventControl appButton5_417msEventControl;
    EmberEventControl appButton5_1667msEventControl;
    EmberEventControl appButton5_EventDetectionReportingControl;

    EmberEventControl appButton6_417msEventControl;
    EmberEventControl appButton6_1667msEventControl;
    EmberEventControl appButton6_EventDetectionReportingControl;

    /*==============================================================================
     *Button1
     *============================================================================*/

    void appButton1_417msEventHandler()
    {

    }

    void appButton1_1667msEventHandler()
    {

    }


    int Button1_Event(int state)
    {

      return 0;
    }


    void appButton1_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton1_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;
      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
                            0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button1_state , Event_button1};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("appButton1_EventDetectionReportingHandler   %d", status);
    }




    /*==========================================================================
     *Button2
     *========================================================================*/

    void appButton2_417msEventHandler()
    {

    }

    void appButton2_1667msEventHandler()
    {

    }


    int Button2_Event(int state)
    {

      return 0;
    }


    void appButton2_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton2_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
                          0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button2_state , Event_button2};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("appButton2_EventDetectionReportingHandler   %d", status);
    }



    /*==========================================================================
     *Button3
     *========================================================================*/


    void appButton3_417msEventHandler()
    {

    }

    void appButton3_1667msEventHandler()
    {

    }


    int Button3_Event(int state)
    {

      return 0;
    }


    void appButton3_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton3_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
          0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button3_state , Event_button3};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);

      emberAfCorePrintln("appButton3_EventDetectionReportingHandler   %d", status);
    }


    /*==========================================================================
     *Button4
     *========================================================================*/


    void appButton4_417msEventHandler()
    {

    }

    void appButton4_1667msEventHandler()
    {

    }


    int Button4_Event(int state)
    {

      return 0;
    }


    void appButton4_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton4_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
          0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button4_state , Event_button4};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);

      emberAfCorePrintln("appButton4_EventDetectionReportingHandler   %d", status);
    }






    /*==========================================================================
     *Button5
     *========================================================================*/


    void appButton5_417msEventHandler()
    {

    }

    void appButton5_1667msEventHandler()
    {

    }


    int Button5_Event(int state)
    {

      return 0;
    }


    void appButton5_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton5_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
          0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button5_state , Event_button5};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("appButton5_EventDetectionReportingHandler   %d", status);
    }




    /*==========================================================================
     *Button6
     *========================================================================*/


    void appButton6_417msEventHandler()
    {

    }

    void appButton6_1667msEventHandler()
    {

    }


    int Button6_Event(int state)
    {

      return 0;
    }


    void appButton6_EventDetectionReportingHandler()
    {
      emberEventControlSetInactive(appButton6_EventDetectionReportingControl);
      EmberAfClusterId clusterId = 0xfc00;

      uint8_t buffer[13] = {MACID[0], MACID[1],MACID[2], MACID[3], MACID[4],
          MACID[5], MACID[6], MACID[7], // mac id
          0x00, 0xc4,     // Attribute_ID
                            0x21,           // data type
                            button6_state , Event_button6};

      uint16_t reportAttributeRecordsLen =  13;


      emberAfFillCommandGlobalServerToClientReportAttributes(clusterId,
                                                             (uint8_t *) buffer,
                                                             reportAttributeRecordsLen);

      emberAfGetCommandApsFrame()->sourceEndpoint      =  Endpoint_1;
      emberAfGetCommandApsFrame()->destinationEndpoint =  Endpoint_1;
      emberAfSetRetryOverride(EMBER_AF_RETRY_OVERRIDE_UNSET);
      EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                DESTINATION_STORTID);
      emberAfCorePrintln("appButton6_EventDetectionReportingHandler   %d", status);
    }






    void set_Button_event(uint8_t active_button, uint8_t active_count)
    {
      if(active_button == 1)
        {
          button1_state = active_count;
          Event_button1 = active_button;
          emberEventControlSetActive(appButton1_EventDetectionReportingControl);
        }

      else if(active_button == 2)
             {
          button2_state = active_count;
          Event_button2 = active_button;
               emberEventControlSetActive(appButton2_EventDetectionReportingControl);
             }

      else if(active_button == 3)
             {
          button3_state = active_count;
          Event_button3 = active_button;
               emberEventControlSetActive(appButton3_EventDetectionReportingControl);
             }

      else if(active_button == 4)
             {
          button4_state = active_count;
          Event_button4 = active_button;
               emberEventControlSetActive(appButton4_EventDetectionReportingControl);
             }

      else if(active_button == 5)
             {
          button5_state = active_count;
          Event_button5 = active_button;
               emberEventControlSetActive(appButton5_EventDetectionReportingControl);
             }

      else if(active_button == 6)
             {
          button6_state = active_count;
          Event_button6 = active_button;
               emberEventControlSetActive(appButton6_EventDetectionReportingControl);
             }
      else if(active_button == 50)
             {
          button1_state = 3;
          Event_button1 = 1;
               emberEventControlSetActive(appButton1_EventDetectionReportingControl);
             }
      else if(active_button == 60)
             {
          button2_state = 3;
          Event_button2 = 2;
               emberEventControlSetActive(appButton2_EventDetectionReportingControl);
             }
      else if(active_button == 70)
             {
          button3_state = 3;
          Event_button3 = 3;
               emberEventControlSetActive(appButton3_EventDetectionReportingControl);
             }
      else if(active_button == 80)
             {
          button4_state = 3;
          Event_button4 = 4;
               emberEventControlSetActive(appButton4_EventDetectionReportingControl);
             }
      else if(active_button == 90)
             {
          button5_state = 3;
          Event_button5 = 5;
               emberEventControlSetActive(appButton5_EventDetectionReportingControl);
             }
      else if(active_button == 100)
             {
          button6_state = 3;
          Event_button6 = 6;
               emberEventControlSetActive(appButton6_EventDetectionReportingControl);
             }

    }
