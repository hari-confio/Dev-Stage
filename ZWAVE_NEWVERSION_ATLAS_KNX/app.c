/**
  * Z-Wave Certified Application Switch On/Off
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */

// this is main project is with dimming and curtain with rx
#include <ecode.h>
#include <stdbool.h>
#include <stdint.h>
#include "Assert.h"
#include "MfgTokens.h"
#include "DebugPrintConfig.h"
#define DEBUGPRINT
#include "DebugPrint.h"
#include "ZW_system_startup_api.h"
#include "zaf_config_security.h"
#include "ZAF_Common_helper.h"
#include "ZAF_Common_interface.h"
#include "ZAF_network_learn.h"
#include "ZAF_network_management.h"
#include "events.h"
#include "zpal_watchdog.h"
#include "app_hw.h"
#include "board_indicator.h"
#include "ZAF_ApplicationEvents.h"
#include "CC_BinarySwitch.h"
#include "zaf_event_distributor_soc.h"
#include "zpal_misc.h"
#include "zw_build_no.h"
#include "zaf_protocol_config.h"
#ifdef DEBUGPRINT
#include "ZAF_PrintAppInfo.h"
#endif
#include <board_config.h>
#include <App_Serial.h>
#include <string.h>
#ifdef DEBUGPRINT
static uint8_t m_aDebugPrintBuffer[96];
#endif
#include <stdio.h>
#include <AppTimer.h>
 #include <CC_Configuration_Parameters.h>
#include <ZAF_file_ids.h>
#include <nvm3.h>
#include <nvm3_default.h>
#include <ZAF_nvm_app.h>
#include <ZAF_nvm.h>

bool btnpress1 = false;
bool btnpress2 = false;
bool btnpress3 = false;

uint8_t btn1level = 0;
uint8_t btn2level = 0;
uint8_t btn3level = 0;
volatile int currentLevel = 0;
int previousKnobLevel = -1;  // Initialize with an invalid level value
uint8_t button_pressed_num = 0;

//unsigned char hexValue = 0;

uint8_t ID[4] = {0};
uint8_t ZONE[4] = {0};
uint16_t CHANNEL[4] = {0};
uint8_t DIMLEVEL[4] = {0};
/// //////
void ApplicationTask(SApplicationHandles* pAppHandles);
void SetDefaultValue( );

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
ZW_APPLICATION_STATUS
ApplicationInit(EResetReason_t eResetReason)
{
  SRadioConfig_t* RadioConfig;

  UNUSED(eResetReason);

  DPRINT("Enabling watchdog\n");
  zpal_enable_watchdog(true);

#ifdef DEBUGPRINT
  zpal_debug_init();
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), zpal_debug_output);
  DebugPrintf("ApplicationInit eResetReason = %d\n", eResetReason);
#endif

  RadioConfig = zaf_get_radio_config();

  // Read Rf region from MFG_ZWAVE_COUNTRY_FREQ
  zpal_radio_region_t regionMfg;
  ZW_GetMfgTokenDataCountryFreq((void*) &regionMfg);
  if (isRfRegionValid(regionMfg)) {
    RadioConfig->eRegion = regionMfg;
  } else {
    ZW_SetMfgTokenDataCountryRegion((void*) &RadioConfig->eRegion);
  }
  DPRINTF("Rf region: %d\n", RadioConfig->eRegion);

  /*************************************************************************************
   * CREATE USER TASKS  -  ZW_ApplicationRegisterTask() and ZW_UserTask_CreateTask()
   *************************************************************************************
   * Register the main APP task function.
   *
   * ATTENTION: This function is the only task that can call ZAF API functions!!!
   * Failure to follow guidelines will result in undefined behavior.
   *
   * Furthermore, this function is the only way to register Event Notification
   * Bit Numbers for associating to given event handlers.
   *
   * ZW_UserTask_CreateTask() can be used to create additional tasks.
   * @see zwave_soc_sensor_pir example for more info.
   *************************************************************************************/
  bool bWasTaskCreated = ZW_ApplicationRegisterTask(
                                                    ApplicationTask,
                                                    EAPPLICATIONEVENT_ZWRX,
                                                    EAPPLICATIONEVENT_ZWCOMMANDSTATUS,
                                                    zaf_get_protocol_config()
                                                    );
  ASSERT(bWasTaskCreated);

  return(APPLICATION_RUNNING);
}


/**
 * A pointer to this function is passed to ZW_ApplicationRegisterTask() making it the FreeRTOS
 * application task.
 */
void
ApplicationTask(SApplicationHandles* pAppHandles)
{
  ZAF_Init(xTaskGetCurrentTaskHandle(), pAppHandles);

#ifdef DEBUGPRINT
  ZAF_PrintAppInfo();
#endif

  app_hw_init();

  /* Protocol will commence SmartStart only if the node is NOT already included in the network */
  ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  DPRINT("ATLAS Event processor Started\r\n");

    ApplicationData1=readAppData();
   for (;;)
  {
    if (!zaf_event_distributor_distribute())
    {
      return;
    }
  }
}

/**
 * @brief The core state machine of this sample application.
 * @param event The event that triggered the call of zaf_event_distributor_app_event_manager.
 */
SSwTimer Button_Event_Running_Timer;
uint8_t button_event_happed = 0;

void create_frame(byte on_off_val,uint16_t destHighByte, uint16_t destLowByte);
void updateAndSendFrame(const SApplicationData1* AppData1, byte newValue) ;
void updateAndSendFrame_1(const SApplicationData1* AppData1, byte newValue);
  void updateAndSendFrame_2(const SApplicationData1* AppData1, byte newValue);


void
zaf_event_distributor_app_event_manager(uint8_t event)
{
  /**************************** PUT YOUR CODE HERE ****************************\
   *
   * This is the main body of the application.
   * You can use the event manager to capture your own custom events
   * (which you can define in events.h) and execute related business logic.
   *
  \****************************************************************************/

  //DPRINTF("zaf_event_distributor_app_event_manager Ev: %d\r\n", event);
  uint8_t destHighByte = 0;
     uint8_t destLowByte = 0;
  uint8_t hexValue = (uint8_t)((rotaryLevel / 100.0) * 255);
  switch (event)
  {


    case APP_EVENT_SCENE_1 :
      {
    //  DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);

      updateAndSendFrame(&ApplicationData1,hexValue);

/*      if(rotaryLevel==5)
                       {
                         button_pressed_num=1;
                        DPRINT("\n IN button_pressed_num 1  \n");
                        }*/

           btnpress1 = true;
            btnpress2 = false;
            btnpress3 = false;

      break;
      }
    case APP_EVENT_SCENE_2:
      {
   //   DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);

      updateAndSendFrame_1(&ApplicationData1,hexValue);
      if(rotaryLevel==50)
          {
           zaf_event_distributor_enqueue_app_event(EVENT_SYSTEM_LEARNMODE_TOGGLE);

               DPRINT("\n IN LEARN MODE \n");
        }
/*      if(rotaryLevel==5)
                       {
                         button_pressed_num=2;
                        DPRINT("\n IN button_pressed_num 2  \n");
                        }*/

             btnpress2 = true;
             btnpress1 = false;
             btnpress3 = false;

      break;
      }
    case APP_EVENT_SCENE_3 :
      {
   //   DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);

      updateAndSendFrame_2(&ApplicationData1,hexValue);
/*      if(rotaryLevel==5)
                       {
                         button_pressed_num=3;
                        DPRINT("\n IN button_pressed_num 3  \n");
                        }*/

              btnpress3 = true;
              btnpress1 = false;
              btnpress2 = false;

      break;
      }
    case APP_EVENT_SCENE_4 :
      {
        DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
        unsigned short knxAddress = (ApplicationData1.dest_main_val_3 << 11) |
                                          (ApplicationData1.dest_mid_val_3 << 8) |
                                          ApplicationData1.dest_sub_val_3;
              destHighByte = (knxAddress >> 8) & 0xFF;
              destLowByte = knxAddress & 0xFF;
              create_frame(128, destHighByte, destLowByte);

        break;
      }
    case APP_EVENT_SCENE_5 :
      {
        DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
        unsigned short knxAddress = (ApplicationData1.dest_main_val_4 << 11) |
                                          (ApplicationData1.dest_mid_val_4 << 8) |
                                          ApplicationData1.dest_sub_val_4;
              destHighByte = (knxAddress >> 8) & 0xFF;
              destLowByte = knxAddress & 0xFF;
              create_frame(128, destHighByte, destLowByte);

        break;
      }
    case APP_EVENT_SCENE_6 :
      {
        DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
        unsigned short knxAddress = (ApplicationData1.dest_main_val_5 << 11) |
                                          (ApplicationData1.dest_mid_val_5 << 8) |
                                          ApplicationData1.dest_sub_val_5;
              destHighByte = (knxAddress >> 8) & 0xFF;
              destLowByte = knxAddress & 0xFF;
              create_frame( 128, destHighByte, destLowByte);

        break;
      }
    /// //////////////////
    case EVENT_APP_SEND_NIF:
      /*
       * This shows how to force the transmission of an Included NIF. Normally, an INIF is
       * transmitted when the product is power cycled while already being included in a network.
       * If the enduser cannot power cycle the product, e.g. because the battery cannot be
       * removed, the transmission of an INIF must be manually triggered.
       */
      DPRINT("TX INIF");
      ZAF_SendINIF(NULL);
      break;

    default:
      // Unknow event - do nothing.
      break;
  }
}


//====================
SApplicationData1 ApplicationData1;
SApplicationData1 AppData1;
void AppResetNvm(void)
{
  DPRINT("Resetting application FileSystem to default\r\n");

  //ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure

  Ecode_t errCode = nvm3_eraseAll(nvm3_defaultHandle);
  ASSERT(ECODE_NVM3_OK == errCode); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure

  /* Apparently there is no valid configuration in the NVM3 file system, so load */
  /* default values and save them. */
  SetDefaultValue();
}

SApplicationData1 readAppData(void)
{
  //ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure
  SApplicationData1 AppData1;

  Ecode_t errCode = nvm3_readData(nvm3_defaultHandle, FILE_ID_APPLICATIONDATA, &AppData1, sizeof(SApplicationData1));
 //DPRINTF("Read Data:\n");
  if(ECODE_NVM3_OK != errCode)
   {
       do
       {
            DPRINTF("Nvm3 App Read Data FAILED %X\n",errCode);
            AppResetNvm();
            errCode = nvm3_readData(nvm3_defaultHandle, FILE_ID_APPLICATIONDATA, &AppData1, sizeof(SApplicationData1));
       }while(ECODE_NVM3_OK != errCode);
   }////


  return AppData1;
}

void writeAppData(const SApplicationData1* pAppData)
{
  Ecode_t errCode = nvm3_writeData(nvm3_defaultHandle, FILE_ID_APPLICATIONDATA, pAppData, sizeof(SApplicationData1));
  UNUSED(errCode);
}



uint16_t GetConfigurationParameterValue(uint16_t parameterNumber)
{
  switch(parameterNumber)
  {


    case 1:
      return ApplicationData1.dest_main_val;
    break;

    case 2:
      return ApplicationData1.dest_mid_val;
    break;

    case 3:
      return ApplicationData1.dest_sub_val;
    break;
    case 4:
      return ApplicationData1.dest_main_val_1;
    break;

    case 5:
      return ApplicationData1.dest_mid_val_1;
    break;

    case 6:
      return ApplicationData1.dest_sub_val_1;
    break;

    case 7:
      return ApplicationData1.dest_main_val_2;
    break;

    case 8:
      return ApplicationData1.dest_mid_val_2;
    break;

    case 9:
      return ApplicationData1.dest_sub_val_2;
    break;


    case 10:
      return ApplicationData1.dest_main_val_3;
    break;

    case 11:
      return ApplicationData1.dest_mid_val_3;
    break;

    case 12:
      return ApplicationData1.dest_sub_val_3;
    break;



    case 13:
      return ApplicationData1.dest_main_val_4;
    break;

    case 14:
      return ApplicationData1.dest_mid_val_4;
    break;

    case 15:
      return ApplicationData1.dest_sub_val_4;
    break;

    case 16:
      return ApplicationData1.dest_main_val_5;
    break;

    case 17:
      return ApplicationData1.dest_mid_val_5;
    break;

    case 18:
      return ApplicationData1.dest_sub_val_5;
    break;


     default:
      return 0xFE;
    break;
  }
  return 0xFE;
}
//

//
uint32_t hexToCustomDecimal(uint32_t hexValue) {
    uint32_t decimalValue = 0;
    uint32_t placeValue = 1; // Tracks the place value (units, tens, etc.)

    // Process each nibble (4 bits) of the hex value
    while (hexValue > 0) {
        uint32_t digit = hexValue & 0xF; // Extract the least significant nibble
        decimalValue += digit * placeValue; // Add it to the result at the appropriate place
        placeValue *= 10; // Shift to the next decimal place
        hexValue >>= 4;   // Remove the processed nibble
    }

    return decimalValue;
}

void  SetConfigurationParameterValue(uint16_t parameterNumber, uint32_t parameterValue)
{
 // UNUSED(parameterValue);
 // DPRINTF("parameterNumber : %d, parameterValue : %d\n",parameterNumber,parameterValue);
//  ApplicationData1.parameter_num = (uint8_t)parameterNumber;
  uint32_t decimalValue = hexToCustomDecimal(parameterValue);
  DPRINTF("Converted Parameter Value (Decimal): %u\n", decimalValue);

  switch(parameterNumber)
  {

    case 1:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val = (uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 2:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val = (uint16_t)decimalValue;
    break;

    case 3:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val = (uint16_t)decimalValue;
    break;
//=================
    //second



    case 4:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_1 = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val_1 = (uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 5:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_1 = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val_1 = (uint16_t)decimalValue;
    break;

    case 6:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_1 = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val_1 = (uint16_t)decimalValue;
    break;
//========================


    case 7:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_2 = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val_2 = (uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 8:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_2 = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val_2 = (uint16_t)decimalValue;
    break;

    case 9:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_2 = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val_2 = (uint16_t)decimalValue;

      break;


    case 10:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_3 = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val_3 = (uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 11:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_3 = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val_3 = (uint16_t)decimalValue;
    break;

    case 12:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_3 = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val_3 = (uint16_t)decimalValue;
    break;

    case 13:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_4 = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val_4 =(uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 14:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_4 = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val_4 =(uint16_t)decimalValue;
    break;

    case 15:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_4 = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val_4 =(uint16_t)decimalValue;
    break;

    case 16:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_5 = (uint16_t)parameterNumber;

      ApplicationData1.dest_main_val_5 =(uint16_t)decimalValue;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

    break;

    case 17:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_5 = (uint16_t)parameterNumber;

      ApplicationData1.dest_mid_val_5 =(uint16_t)decimalValue;
    break;

    case 18:
//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_5 = (uint16_t)parameterNumber;

      ApplicationData1.dest_sub_val_5 =(uint16_t)decimalValue;
    break;
//==================

    default:
   //   ApplicationData.parameter_num = (uint8_t)parameterNumber;
      break;
  }
   writeAppData(&ApplicationData1);
}

//===================================

//===================================
void SetDefaultValue( )
{
  DPRINT("---default---");



 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num =1;
      ApplicationData1.dest_main_val = 9;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);


 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num =2;
      ApplicationData1.dest_mid_val = 3;

 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num =3;
     ApplicationData1.dest_sub_val= 1;


 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_1 =4;
   ApplicationData1.dest_main_val_1 = 9;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);

//      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_1 =5;
    ApplicationData1.dest_mid_val_1 = 3;

 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_1 =6;
  ApplicationData1.dest_sub_val_1 = 2;


 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_main_num_2 =7;
      ApplicationData1.dest_main_val_2 = 9;
//     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);


 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_mid_num_2 =8;
      ApplicationData1.dest_mid_val_2 = 3;

 //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
      ApplicationData1.dest_sub_num_2 =9;
      ApplicationData1.dest_sub_val_2 = 3;



      ApplicationData1.dest_main_num_3 =10;
       ApplicationData1.dest_main_val_3 = 9;
 //     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);


  //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
       ApplicationData1.dest_mid_num_3 =11;
       ApplicationData1.dest_mid_val_3 = 3;

  //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
       ApplicationData1.dest_sub_num_3 =12;
       ApplicationData1.dest_sub_val_3 = 4;
       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_main_num_4 =13;

            ApplicationData1.dest_main_val_4 = 9;
      //     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);


       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_mid_num_4 =14;

            ApplicationData1.dest_mid_val_4 = 3;

       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_sub_num_4 =15;

            ApplicationData1.dest_sub_val_4 = 5;

       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_main_num_5 =16;

            ApplicationData1.dest_main_val_5 = 9;
      //     DPRINTF("ApplicationData: %d\n",ApplicationData.dest_main_val);


       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_mid_num_5 =17;

            ApplicationData1.dest_mid_val_5 = 3;

       //      DPRINTF("parameterNumber : %d, \n",parameterNumber);
            ApplicationData1.dest_sub_num_5 =18;

            ApplicationData1.dest_sub_val_5 = 6;


writeAppData(&ApplicationData1);
}
//=============================================================

//=============================================================



void knx_frame_rem(bool btn1, bool btn2, bool btn3)
{

 // uint8_t hexValue = (uint8_t)((0 / 100.0) * 255);
          //  DPRINTF("\n Rotary Level: %d, Hex Value: 0x%02X \n", rotaryLevel, hexValue);
            if(btn1)
             {
                btn1level = (btn1level == 0) ? 100 : 0;

            //    DPRINTF(" FIRST 0 per \n");
                updateAndSendFrame(&ApplicationData1,btn1level);
                updateKnobLevelhex(btn1level);

             }
            if(btn2)
              {
              //  DPRINTF(" sec 0 per \n");
                btn2level = (btn2level == 0) ? 100 : 0;

                updateAndSendFrame_1(&ApplicationData1,btn2level);
                updateKnobLevelhex(btn2level);


              }
            if(btn3)
                {
             //   DPRINTF(" third 0 per \n");
                btn3level = (btn3level == 0) ? 100 : 0;

                  updateAndSendFrame_2(&ApplicationData1,btn3level);
                  updateKnobLevelhex(btn3level);

                }

}
byte CalculateChecksum(byte frame[], byte size) {
    byte xorSum = 0;
    for (byte i = 0; i < size; i++) {
        xorSum ^= frame[i];
    }
    return (byte)(~xorSum);
}

void updateAndSendFrame(const SApplicationData1* AppData1, byte newValue) {
//    // Define the frame (excluding checksum)
//    DPRINTF("source_main_val: %d\n", AppData1->source_main_val);
//    DPRINTF("source_mid_val: %d\n", AppData1->source_mid_val);
//   DPRINTF("source_sub_val: %d\n", AppData1->source_sub_val);
//   DPRINTF("dest_main_val: %d\n", AppData1->dest_main_val);
//    DPRINTF("dest_mid_val: %d\n", AppData1->dest_mid_val);
//   DPRINTF("dest_sub_val: %d\n", AppData1->dest_sub_val);
    byte testFrame[] = {0xBC, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84, 0x00, 0x85, 0x92, 0x80, 0x80};
    byte frameSize = sizeof(testFrame) / sizeof(testFrame[0]);

    // Update the last value in the frame with the new value


    // Calculate the checksum
   // byte checksum = CalculateChecksum(testFrame, frameSize);
    byte txFrame50[] = {
         0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
         0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x80,
         0x49, 0xDF
     };
    byte frameSize1 = sizeof(txFrame50) / sizeof(txFrame50[0]);

    unsigned short knxAddress = (AppData1->dest_main_val << 11) | (AppData1->dest_mid_val << 8) | AppData1->dest_sub_val;

        // Print the binary representation (optional for debugging)
      //  DPRINTF("Binary KNX Address: ");
        for (int i = 15; i >= 0; i--) {
           // DPRINTF("%d", (knxAddress >> i) & 1);
        }
      //  DPRINTF("\n");
        uint8_t destHighByte = (knxAddress >> 8) & 0xFF;  // Extract the high byte
          uint8_t destLowByte = knxAddress & 0xFF;
        // Print the hexadecimal representation
      //  DPRINTF("Hexadecimal KNX Address: 0x%04X\n", knxAddress);
        uint8_t sourceByte1 = (1 << 4) | (5 & 0x0F); // 4 bits for main, 4 bits for mid
           uint8_t sourceByte2 = 8;  //

           testFrame[frameSize - 1] = newValue;
           testFrame[1] = sourceByte1;

          // testFrame[2] = (txFrame50[2] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
           testFrame[3] = sourceByte2 ; // Replace third byte's high nibble with sourceByte2

           testFrame[5] = destHighByte; // Replace eighth byte with destHighByte
           testFrame[7] = destLowByte;  // Replace tenth byte with destLowByte
           byte checksum = CalculateChecksum(testFrame, frameSize);
           for (byte i = 0; i < frameSize; i++) {
              //  DPRINTF("0x%02X ", testFrame[i]);
            }
    // Update the last byte in the frame with the checksum value
    txFrame50[frameSize1 - 3] = newValue;  // 18th byte (index 18 in 0-based index)
    txFrame50[frameSize1 - 1] = checksum;
    txFrame50[3] = sourceByte1; // Replace fourth byte with sourceByte1
    txFrame50[5] = sourceByte2; // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
    txFrame50[7] = destHighByte; // Replace eighth byte with destHighByte
    txFrame50[9] = destLowByte;  // Replace tenth byte with destLowByte
   // DPRINTF("Updated Frame: ");
    for (byte i = 0; i < frameSize1; i++) {
     //   DPRINTF("0x%02X ", txFrame50[i]);
    }
 //   DPRINTF("\n");
    // Print the calculated checksum
   // DPRINTF("New Value: 0x%02X, Calculated Checksum: %02X\n", newValue, checksum);
    sendFrame(txFrame50);

}
//================================
//**************************************************

void updateAndSendFrame_1(const SApplicationData1* AppData1, byte newValue) {
  //   Define the frame (excluding checksum)
//    DPRINTF("source_main_val_1: %d\n", AppData1->source_main_val_1);
//    DPRINTF("source_mid_val: %d\n", AppData1->source_mid_val_1);
//   DPRINTF("source_sub_val: %d\n", AppData1->source_sub_val_1);
//   DPRINTF("dest_main_val: %d\n", AppData1->dest_main_val_1);
//   DPRINTF("dest_mid_val: %d\n", AppData1->dest_mid_val_1);
//  DPRINTF("dest_sub_val: %d\n", AppData1->dest_sub_val_1);
    byte testFrame[] = {0xBC, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84, 0x00, 0x85, 0x92, 0x80, 0x80};
    byte frameSize = sizeof(testFrame) / sizeof(testFrame[0]);

    // Update the last value in the frame with the new value


    // Calculate the checksum
   // byte checksum = CalculateChecksum(testFrame, frameSize);
    byte txFrame50[] = {
         0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
         0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x80,
         0x49, 0xDF
     };
    byte frameSize1 = sizeof(txFrame50) / sizeof(txFrame50[0]);



    unsigned short knxAddress = (AppData1->dest_main_val_1 << 11) | (AppData1->dest_mid_val_1 << 8) | AppData1->dest_sub_val_1;

        // Print the binary representation (optional for debugging)
      //  DPRINTF("Binary KNX Address: ");
        for (int i = 15; i >= 0; i--) {
           // DPRINTF("%d", (knxAddress >> i) & 1);
        }
   //     DPRINTF("\n");
        uint8_t destHighByte = (knxAddress >> 8) & 0xFF;  // Extract the high byte
          uint8_t destLowByte = knxAddress & 0xFF;
        // Print the hexadecimal representation
      //  DPRINTF("Hexadecimal KNX Address: 0x%04X\n", knxAddress);
        uint8_t sourceByte1 = (1 << 4) | (5 & 0x0F); // 4 bits for main, 4 bits for mid
           uint8_t sourceByte2 = 8;  //

           testFrame[frameSize - 1] = newValue;
           testFrame[1] = sourceByte1;
          // testFrame[2] = (txFrame50[2] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
           testFrame[3] = sourceByte2; // Replace third byte's high nibble with sourceByte2

           testFrame[5] = destHighByte; // Replace eighth byte with destHighByte
           testFrame[7] = destLowByte;  // Replace tenth byte with destLowByte
           byte checksum = CalculateChecksum(testFrame, frameSize);
           for (byte i = 0; i < frameSize; i++) {
              //  DPRINTF("0x%02X ", testFrame[i]);
            }
    // Update the last byte in the frame with the checksum value
    txFrame50[frameSize1 - 3] = newValue;  // 18th byte (index 18 in 0-based index)
    txFrame50[frameSize1 - 1] = checksum;
    txFrame50[3] = sourceByte1; // Replace fourth byte with sourceByte1
//    txFrame50[4] = (txFrame50[4] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
       txFrame50[5] = sourceByte2; // Replace fifth byte high nibble with sourceByte2, keep low nibble as is

    txFrame50[7] = destHighByte; // Replace eighth byte with destHighByte
    txFrame50[9] = destLowByte;  // Replace tenth byte with destLowByte
   // DPRINTF("Updated Frame: ");
    for (byte i = 0; i < frameSize1; i++) {
      //  DPRINTF("0x%02X ", txFrame50[i]);
    }
//    DPRINTF("\n");
    // Print the calculated checksum
   // DPRINTF("New Value: 0x%02X, Calculated Checksum: %02X\n", newValue, checksum);
    sendFrame(txFrame50);

}

void updateAndSendFrame_2(const SApplicationData1* AppData1, byte newValue) {

    byte testFrame[] = {0xBC, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84, 0x00, 0x85, 0x92, 0x80, 0x80};
    byte frameSize = sizeof(testFrame) / sizeof(testFrame[0]);

    // Update the last value in the frame with the new value


    // Calculate the checksum
   // byte checksum = CalculateChecksum(testFrame, frameSize);
    byte txFrame50[] = {
         0x80, 0xBC, 0x81, 0x15, 0x82, 0x08, 0x83, 0x13, 0x84,
         0x00, 0x85, 0x92, 0x86, 0x00, 0x87, 0x80, 0x88, 0x80,
         0x49, 0xDF
     };
    byte frameSize1 = sizeof(txFrame50) / sizeof(txFrame50[0]);



    unsigned short knxAddress = (AppData1->dest_main_val_2 << 11) | (AppData1->dest_mid_val_2 << 8) | AppData1->dest_sub_val_2;

        // Print the binary representation (optional for debugging)
      //  DPRINTF("Binary KNX Address: ");
        for (int i = 15; i >= 0; i--) {
           // DPRINTF("%d", (knxAddress >> i) & 1);
        }
     //   DPRINTF("\n");
        uint8_t destHighByte = (knxAddress >> 8) & 0xFF;  // Extract the high byte
          uint8_t destLowByte = knxAddress & 0xFF;
        // Print the hexadecimal representation
      //  DPRINTF("Hexadecimal KNX Address: 0x%04X\n", knxAddress);
        uint8_t sourceByte1 = (1 << 4) | (5 & 0x0F); // 4 bits for main, 4 bits for mid
           uint8_t sourceByte2 = 8;  //

           testFrame[frameSize - 1] = newValue;
           testFrame[1] = sourceByte1;
          // testFrame[2] = (txFrame50[2] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
           testFrame[2] = (testFrame[2] & 0x0F) | (sourceByte2 << 4); // Replace third byte's high nibble with sourceByte2

           testFrame[5] = destHighByte; // Replace eighth byte with destHighByte
           testFrame[7] = destLowByte;  // Replace tenth byte with destLowByte
           byte checksum = CalculateChecksum(testFrame, frameSize);
           for (byte i = 0; i < frameSize; i++) {
           //     DPRINTF("0x%02X ", testFrame[i]);
            }
    // Update the last byte in the frame with the checksum value
    txFrame50[frameSize1 - 3] = newValue;  // 18th byte (index 18 in 0-based index)
    txFrame50[frameSize1 - 1] = checksum;
    txFrame50[3] = sourceByte1; // Replace fourth byte with sourceByte1
    txFrame50[4] = (txFrame50[4] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
    txFrame50[7] = destHighByte; // Replace eighth byte with destHighByte
    txFrame50[9] = destLowByte;  // Replace tenth byte with destLowByte
 //   DPRINTF("Updated Frame: ");
    for (byte i = 0; i < frameSize1; i++) {
  //      DPRINTF("0x%02X ", txFrame50[i]);
    }
//  DPRINTF("\n");
    // Print the calculated checksum
   // DPRINTF("New Value: 0x%02X, Calculated Checksum: %02X\n", newValue, checksum);
    sendFrame(txFrame50);

}
void extractDestinationValues(uint8_t destHighByte, uint8_t destLowByte, uint8_t dim_value) {

  unsigned short knxAddress = (destHighByte << 8) | destLowByte;
 // DPRINTF(" matching scene fun \n");

    // Extract dest_main_val, dest_mid_val, and dest_sub_val
    uint8_t dest_main_value = (knxAddress >> 11) & 0x1F;  // 5 bits for main value
    uint8_t dest_mid_value = (knxAddress >> 8) & 0x07;   // 3 bits for mid value
    uint8_t dest_sub_value = knxAddress & 0xFF;          // 8 bits for sub value


    if (dest_main_value == ApplicationData1.dest_main_val &&
          dest_mid_value == ApplicationData1.dest_mid_val &&
          dest_sub_value == ApplicationData1.dest_sub_val) {
        zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_1);

        //  DPRINTF("Matched Scene 1\n");
        btn1level=((dim_value * 100) / 255);
          DPRINTF("Matched Scene-1 level 1 is %u \n",btn1level);
          DPRINTF("Matched Scene-1 level 2 is %u \n",btn2level);
          DPRINTF("Matched Scene-1 level 3 is %u \n",btn3level);

        updateKnobLevelhex(btn1level);


      }
       if (dest_main_value == ApplicationData1.dest_main_val_1 &&
               dest_mid_value == ApplicationData1.dest_mid_val_1 &&
               dest_sub_value == ApplicationData1.dest_sub_val_1) {
//          DPRINTF("Matched Scene 2\n");
           zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_2);

           btn2level=((dim_value * 100) / 255);
           DPRINTF("Matched Scene-2 level 1 is %u \n",btn1level);
           DPRINTF("Matched Scene-2 level 2 is %u \n",btn2level);
           DPRINTF("Matched Scene-2 level 3 nis %u \n",btn3level);

              updateKnobLevelhex(btn2level);
//
//                     DPRINTF("dest_main_val2: %d\n", appData2.dest_main_val_1);
//                      DPRINTF("dest_mid_val: %d\n", appData2.dest_mid_val_1);
//                     DPRINTF("dest_sub_val: %d\n", appData2.dest_sub_val_1);
//
//                  // Print the results
//                  DPRINTF("extracted dest_main_val: %d\n", dest_main_value);
//                  DPRINTF("dest_mid_val: %d\n", dest_mid_value);
//                  DPRINTF("ex dest_sub_val: %d\n", dest_sub_value);
      }
       if (dest_main_value == ApplicationData1.dest_main_val_2 &&
               dest_mid_value == ApplicationData1.dest_mid_val_2 &&
               dest_sub_value == ApplicationData1.dest_sub_val_2) {
//          DPRINTF("Matched Scene 3\n");
           zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_SCENE_3);

           btn3level=((dim_value * 100) / 255);
           DPRINTF("Matched Scene-3 level 1 is %u \n",btn1level);
           DPRINTF("Matched Scene-3 level 2 is  %u \n",btn2level);
           DPRINTF("Matched Scene-3 level 3  is %u \n",btn3level);

              updateKnobLevelhex(btn3level);

//                     DPRINTF("dest_main_val3: %d\n", appData2.dest_main_val_2);
//                      DPRINTF("dest_mid_val: %d\n", appData2.dest_mid_val_2);
//                     DPRINTF("dest_sub_val: %d\n", appData2.dest_sub_val_2);
//
//                  // Print the results
//                  DPRINTF("extracted dest_main_val: %d\n", dest_main_value);
//                  DPRINTF("dest_mid_val: %d\n", dest_mid_value);
//                  DPRINTF("ex dest_sub_val: %d\n", dest_sub_value);
      }
      else {
        //  DPRINTF("No matching scene found\n");
      }
}
//=================================================================================


bool parametersReceivedFromEts = false; // Tracks if parameters were received from ETS
void set_default_ets_param(uint8_t dest_main, uint8_t dest_middle, uint8_t dest_sub, uint8_t index) {
    // Store values into ApplicationData1 based on the index
  DPRINTF("*dest_main_%d : %u, dest_middle_%d : %u, dest_sub_%d : %u\n", index + 1, dest_main, index + 1, dest_middle, index + 1, dest_sub);
  uint8_t mapped_index = index % 6;  // This maps any index to the range 0-5

/*    ApplicationData1.source_main_num = 1;
    ApplicationData1.source_main_val = 1;
    ApplicationData1.source_mid_num = 2;
    ApplicationData1.source_mid_val = 5;
    ApplicationData1.source_sub_num = 3;
    ApplicationData1.source_sub_val = 8;*/

    switch (mapped_index) {
        case 0:
            ApplicationData1.dest_main_num = 1;
            ApplicationData1.dest_main_val = dest_main;
            ApplicationData1.dest_mid_num = 2;
            ApplicationData1.dest_mid_val = dest_middle;
            ApplicationData1.dest_sub_num = 3;
            ApplicationData1.dest_sub_val = dest_sub;
            break;
        case 1:
            ApplicationData1.dest_main_num_1 = 4;
            ApplicationData1.dest_main_val_1 = dest_main;
            ApplicationData1.dest_mid_num_1 = 5;
            ApplicationData1.dest_mid_val_1 = dest_middle;
            ApplicationData1.dest_sub_num_1 = 6;
            ApplicationData1.dest_sub_val_1 = dest_sub;
            break;
        case 2:
            ApplicationData1.dest_main_num_2 = 7;
            ApplicationData1.dest_main_val_2 = dest_main;
            ApplicationData1.dest_mid_num_2 = 8;
            ApplicationData1.dest_mid_val_2 = dest_middle;
            ApplicationData1.dest_sub_num_2 = 9;
            ApplicationData1.dest_sub_val_2 = dest_sub;
            break;
        case 3:
            ApplicationData1.dest_main_num_3 = 10;
            ApplicationData1.dest_main_val_3 = dest_main;
            ApplicationData1.dest_mid_num_3 = 11;
            ApplicationData1.dest_mid_val_3 = dest_middle;
            ApplicationData1.dest_sub_num_3 = 12;
            ApplicationData1.dest_sub_val_3 = dest_sub;
            break;
        case 4:
            ApplicationData1.dest_main_num_4 = 13;
            ApplicationData1.dest_main_val_4 = dest_main;
            ApplicationData1.dest_mid_num_4 = 14;
            ApplicationData1.dest_mid_val_4 = dest_middle;
            ApplicationData1.dest_sub_num_4 = 15;
            ApplicationData1.dest_sub_val_4 = dest_sub;
            break;
        case 5:
            ApplicationData1.dest_main_num_5 = 16;
            ApplicationData1.dest_main_val_5 = dest_main;
            ApplicationData1.dest_mid_num_5 = 17;
            ApplicationData1.dest_mid_val_5 = dest_middle;
            ApplicationData1.dest_sub_num_5 = 18;
            ApplicationData1.dest_sub_val_5 = dest_sub;
            break;
        default:
            DPRINTF("Invalid index %d.\n", index);
            break;
    }


    writeAppData(&ApplicationData1);
    //readAppData();
   // Set_Led(mapped_index+1,1);
     if(mapped_index==5)
       {
         DPRINTF("=============================MAKE IT 0 NOW ================\n");
         button_pressed_num=0;
       }

}

void extract_and_print_group_addresses(uint8_t *buffer, uint8_t length, uint8_t button_num) {
  // Check if length is valid and there is at least one group address
  if (length == 0) {
      DPRINTF("No group addresses found.\n");
      return;
  }
  parametersReceivedFromEts = true;

  // Calculate the maximum number of sets (each set contains 6 group addresses)
  uint8_t max_sets = (length + 5) / 6; // This handles up to 4 sets, even if there's a partial set at the end

  // Ensure the button_press_count is between 1 and 4, as there can be a maximum of 4 sets
  if (button_num < 1 || button_num > 4) {
      DPRINTF("Invalid button press count. Valid values are 1 to 4.\n");
      return;
  }

  // Start printing from the correct set based on button_press_count
  uint8_t start_index = (button_num - 1) * 6;
  uint8_t end_index = start_index + 6;
  if (end_index > length) {
      end_index = length; // Avoid exceeding the available group addresses
  }

  DPRINTF("Group Addresses (Set %d):\n", button_num);

  // Iterate over the group addresses in the selected set
  for (int i = start_index; i < end_index; i++) {
      uint16_t group_address = (buffer[13 + (2 * i)] << 8) | buffer[14 + (2 * i)];
    // DPRINTF("Group Address %d: 0x%.4X\n", i + 1, group_address);

      // Extract main, middle, and sub components
      uint8_t dest_main = (group_address >> 11) & 0x1F;  // Extract top 5 bits (main)
      uint8_t dest_middle = (group_address >> 8) & 0x07; // Extract next 3 bits (middle)
      uint8_t dest_sub = group_address & 0xFF;           // Extract lower 8 bits (sub)

      if (button_num <= max_sets) {
              if(parametersReceivedFromEts)
                {
                    set_default_ets_param(dest_main, dest_middle, dest_sub, i); // Store the extracted values
                }
          }
      // Print the extracted values
     // DPRINTF("dest_main_%d : %u, dest_middle_%d : %u, dest_sub_%d : %u\n", i + 1, dest_main, i + 1, dest_middle, i + 1, dest_sub);
  }

  // Indicate if we've reached the end of the selected set
  if (end_index < length) {
      DPRINTF("------ End of Set %d ------\n", button_num);
  } else {
      DPRINTF("------ End of Final Set ------\n");
  }

}


void extract_and_print_group_addresses_1(uint8_t *buffer, uint8_t length, uint8_t button_num) {
  // Check if length is valid and there is at least one group address
  if (length == 0) {
      DPRINTF("No group addresses found.\n");
      return;
  }
  parametersReceivedFromEts = true;

  // Calculate the maximum number of sets (each set contains 6 group addresses)
  uint8_t max_sets = (length + 5) / 6; // This handles up to 4 sets, even if there's a partial set at the end

  // Ensure the button_press_count is between 1 and 4, as there can be a maximum of 4 sets
  if (button_num < 1 || button_num > 4) {
      DPRINTF("Invalid button press count. Valid values are 1 to 4.\n");
      return;
  }

  // Start printing from the correct set based on button_press_count
  uint8_t start_index = (button_num - 1) * 6;
  uint8_t end_index = start_index + 6;
  if (end_index > length) {
      end_index = length; // Avoid exceeding the available group addresses
  }

  DPRINTF("Group Addresses (Set %d):\n", button_num);

  // Iterate over the group addresses in the selected set
  for (int i = start_index; i < end_index; i++) {
      uint16_t group_address = (buffer[12 + (2 * i)] << 8) | buffer[13 + (2 * i)];
    // DPRINTF("Group Address %d: 0x%.4X\n", i + 1, group_address);

      // Extract main, middle, and sub components
      uint8_t dest_main = (group_address >> 11) & 0x1F;  // Extract top 5 bits (main)
      uint8_t dest_middle = (group_address >> 8) & 0x07; // Extract next 3 bits (middle)
      uint8_t dest_sub = group_address & 0xFF;           // Extract lower 8 bits (sub)

      if (button_num <= max_sets) {
              if(parametersReceivedFromEts)
                {
                    set_default_ets_param(dest_main, dest_middle, dest_sub, i); // Store the extracted values
                }
          }
      // Print the extracted values
    //  DPRINTF("dest_main_%d : %u, dest_middle_%d : %u, dest_sub_%d : %u\n", i + 1, dest_main, i + 1, dest_middle, i + 1, dest_sub);
  }

  // Indicate if we've reached the end of the selected set
  if (end_index < length) {
      DPRINTF("------ End of Set %d ------\n", button_num);
  } else {
      DPRINTF("------ End of Final Set ------\n");
  }

}



//============
void create_frame(byte on_off_val,uint16_t destHighByte, uint16_t destLowByte)
{
  byte testFrame[] = {0xBC, 0x15, 0x82, 0x09, 0x83, 0x00, 0x84, 0x04, 0x85, 0x91, 0x00, 0x80};
    byte frameSize = sizeof(testFrame) / sizeof(testFrame[0]);

    // Calculate the checksum
   // byte checksum = CalculateChecksum(testFrame, frameSize);
    byte txFrame1on[] = {
        0x80, 0xBC, 0x81, 0x15, 0x82, 0x09, 0x83, 0x00,
        0x84, 0x01, 0x85, 0x91, 0x86, 0x00, 0x87, 0x81,
        0x48, 0x4E
     };
    byte frameSize1 = sizeof(txFrame1on) / sizeof(txFrame1on[0]);

        // uint8_t destHighByte = (knxAddress >> 8) & 0xFF;  // Extract the high byte
         // uint8_t destLowByte = knxAddress & 0xFF;
        // Print the hexadecimal representation
      //  DPRINTF("Hexadecimal KNX Address: 0x%04X\n", knxAddress);
        uint8_t sourceByte1 = (1 << 4) | (5 & 0x0F); // 4 bits for main, 4 bits for mid
           uint8_t sourceByte2 = 8;  //

//           testFrame[7] = switch_num;
           testFrame[1] = sourceByte1;
          // testFrame[2] = (txFrame1on[2] & 0x0F) | (sourceByte2 << 4); // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
           testFrame[3] =   sourceByte2 ;

           testFrame[5] = destHighByte; // Replace eighth byte with destHighByte
           testFrame[7] = destLowByte;  // Replace tenth byte with destLowByte
           testFrame[frameSize-1] = on_off_val;  // Replace tenth byte with destLowByte

           byte checksum = CalculateChecksum(testFrame, frameSize);
//           for (byte i = 0; i < frameSize; i++) {
//                DPRINTF("0x%02X ", testFrame[i]);
//            }
    // Update the last byte in the frame with the checksum value
    txFrame1on[frameSize1 - 1] = checksum;
    txFrame1on[3] = sourceByte1; // Replace fourth byte with sourceByte1
    txFrame1on[5] = sourceByte2; // Replace fifth byte high nibble with sourceByte2, keep low nibble as is
    txFrame1on[7] = destHighByte; // Replace eighth byte with destHighByte
    txFrame1on[9] = destLowByte;  // Replace tenth byte with destLowByte
    txFrame1on[frameSize1-3] = on_off_val;  // Replace tenth byte with destLowByte

   // DPRINTF("Updated Frame: ");
  //  for (byte i = 0; i < frameSize1; i++) {
    //    DPRINTF("0x%02X ", txFrame1on[i]);
    //}
  //  DPRINTF("\n");
    // Print the calculated checksum
   // DPRINTF("New Value: 0x%02X, Calculated Checksum: %02X\n", switch_num, checksum);
    sendFrame(txFrame1on);
}

