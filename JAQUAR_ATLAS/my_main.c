/**
 * Z-Wave Certified Application Power Strip
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
//#include "config_rf.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "SizeOf.h"
#include "Assert.h"
#include "DebugPrintConfig.h"
#if defined(APP_DEBUG)
  #define DEBUGPRINT
#endif
#include "DebugPrint.h"
//#include "config_app.h"
//#include "ZAF_app_version.h"
#include "ZAF_types.h"
#include <ZAF_file_ids.h>
#include "nvm3.h"
//#include "ZAF_nvm3_app.h"
#include <em_system.h>

#include <ZW_slave_api.h>
#include <ZW_classcmd.h>
#include <ZW_TransportLayer.h>

#include <ZAF_uart_utils.h>

#include <misc.h>


/*IO control*/
#include <board_config.h>

#include <ev_man.h>
#include <AppTimer.h>
#include <SwTimer.h>
#include <EventDistributor.h>
#include <ZW_system_startup_api.h>
#include <ZW_application_transport_interface.h>

#include <association_plus.h>
//#include <agi.h>
#include <CC_Association.h>
#include <CC_AssociationGroupInfo.h>
#include <CC_Basic.h>
#include <CC_BinarySwitch.h>
//#include <CC_Configuration.h>
#include <CC_DeviceResetLocally.h>
#include <CC_Indicator.h>
//#include <CC_MultiChan.h>
//#include <CC_MultiChanAssociation.h>
//#include <CC_MultilevelSwitch_Support.h>
//#include <CC_PowerLevel.h>
//#include <CC_Security.h>
//#include <CC_Supervision.h>
//#include <CC_Version.h>
//#include <CC_ZWavePlusInfo.h>
//#include <CC_FirmwareUpdate.h>
//#include <CC_ManufacturerSpecific.h>
//#include <CC_CentralScene.h>

#include "zaf_event_distributor_soc.h"
#include "zaf_job_helper.h"
#include <ZAF_Common_helper.h>
#include <ZAF_network_learn.h>
#include "ZAF_TSE.h"
//#include <ota_util.h>
#include <ZAF_CmdPublisher.h>
#include <em_wdog.h>
#include "em_timer.h"
#include "em_cmu.h"
#include <App_Serial.h>
/****************************************************************************/
/* Application specific button and LED definitions                          */
/****************************************************************************/

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef enum _STATE_APP_
{
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_RESET,
  STATE_APP_TRANSMIT_DATA
} STATE_APP;

SApplicationData  ApplicationData;

#define APP_EVENT_QUEUE_SIZE 5


SSwTimer App_HW_Init_Timer;
void App_HW_Init_Timer_CB();

//SSwTimer Send_Report_Timer;
//void Send_Report_Timer_CB();

//SSwTimer Button_Event_Running_Timer;
//void Button_Event_Running_Timer_CB();

//static nvm3_Handle_t* pFileSystemApplication;

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void DeviceResetLocally(void);
//STATE_APP GetAppState(void);
//void AppStateManager(EVENT_APP event);
//static void ChangeState(STATE_APP newState);

//static void ApplicationTask(SApplicationHandles* pAppHandles);

void LoadConfiguration(void);
void SetDefaultConfiguration(void);
//void AppResetNvm(void);
//SApplicationData readAppData(void);
//void writeAppData(const SApplicationData* pAppData);

void SetFactoryDefaultConfiguration();

//static void LearnCompleted(void);

void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);

//void ZCB_CommandClassSupervisionGetReceived(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs);
//void Send_Report(uint8_t endpoint);
//
///**
// * @brief Send reset notification.
// */
//void DeviceResetLocally(void)
//{
//  AGI_PROFILE lifelineProfile = {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
//                   ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE};
//
//  DPRINT("Call locally reset\r\n");
//
//  CC_DeviceResetLocally_notification_tx(&lifelineProfile, DeviceResetLocallyDone);
//}
//
///*
// * See description for function prototype in CC_Version.h.
// */
//uint8_t handleNbrFirmwareVersions()
//{
//  return 1; /*CHANGE THIS - firmware 0 version*/
//}
//
//void AppResetNvm(void)
//{
//  DPRINT("Resetting application FileSystem to default\r\n");
//
//  //ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure
//
//  Ecode_t errCode = nvm3_eraseAll(pFileSystemApplication);
//  ASSERT(ECODE_NVM3_OK == errCode); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure
//
//  /* Apparently there is no valid configuration in the NVM3 file system, so load */
//  /* default values and save them. */
//  SetDefaultConfiguration();
//}
////
//
///**
// * @brief Reads application data from file system.
// */
//SApplicationData readAppData(void)
//{
//  //ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure
//  SApplicationData AppData;
//
//  Ecode_t errCode = nvm3_readData(pFileSystemApplication, FILE_ID_APPLICATIONDATA, &AppData, sizeof(SApplicationData));
//  if(ECODE_NVM3_OK != errCode)
//  {
//      do
//      {
//           DPRINTF("Nvm3 App Read Data FAILED %X\n",errCode);
//           AppResetNvm();
//           errCode = nvm3_readData(pFileSystemApplication, FILE_ID_APPLICATIONDATA, &AppData, sizeof(SApplicationData));
//      }while(ECODE_NVM3_OK != errCode);
//  }
////  ASSERT(ECODE_NVM3_OK == errCode); //Assert has been kept for debugging , can be removed from production code. This error hard to occur when a corresponing write is successfull
//                  //Can only happended in case of some hardware failure
//
//  return AppData;
//}
//
///**
// * @brief Writes application data to file system.
// */
//void writeAppData(const SApplicationData* pAppData)
//{
//  //ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This means nvm3_open failed can only be caused by some internal flash HW failure
//
//  Ecode_t errCode = nvm3_writeData(pFileSystemApplication, FILE_ID_APPLICATIONDATA, pAppData, sizeof(SApplicationData));
//  ASSERT(ECODE_NVM3_OK == errCode); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure
//}
//
/**
 * @brief See description for function prototype in CC_CentralScene.h.
 */
//uint8_t getAppCentralSceneReportData(ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V3_FRAME * pData)
//{
//  pData->supportedScenes = 3; // Number of buttons
//  pData->properties1 = (1 << 1) | 1; // 1 bit mask byte & All keys are identical.
//  pData->variantgroup1.supportedKeyAttributesForScene1 = 0x01; // 0b00000111.
//  return 1;
//}
//
//void SetConfigParamDefaultValue(uint16_t ParameterNumber)
//{
//  switch(ParameterNumber)
//  {
//
//    default:
//    break;
//  }
//  writeAppData(&ApplicationData);
//}


// Set the configuration parameter values
//void  SetConfigParamValue(uint16_t parameterNumber, uint32_t parameterValue)
//{
//  UNUSED(parameterNumber);
//  UNUSED(parameterValue);
//
//  DPRINTF("parameterNumber : %d, parameterValue : %d\n",parameterNumber,parameterValue);
//  switch(parameterNumber)
//  {
//
//
//    default:
//    break;
//  }
//  writeAppData(&ApplicationData);
//}
//
//bool IsConfigParamAltering(uint16_t parameterNumber)
//{
//  //CT4R param are NOT changing any protocol settings
//  UNUSED(parameterNumber);
//  return false;
//}
//bool IsConfigParamReadOnly(uint16_t parameterNumber)
//{
//  //CT4R all param are writable
//  UNUSED(parameterNumber);
//  return false;
//}
//uint8_t GetConfigParamType(uint16_t parameterNumber)
//{
//  //CT4R all param are unsigned ints
//  UNUSED(parameterNumber);
//  return CONFIGURATION_PROPERTIES_REPORT_FORMAT_UNSIGNED_INTEGER_V4;
//}
//uint8_t GetConfigParamSize(uint16_t parameterNumber)
//{
//  //CT4R all param are same size (1Byte)
//  UNUSED(parameterNumber);
//  return 0x01;
//}
//uint8_t GetConfigParamMinValue(uint16_t parameterNumber)
//{
//  UNUSED(parameterNumber);
//  return 0;
//}
//uint8_t GetConfigParameterMaxValue(uint16_t parameterNumber)
//{
//  UNUSED(parameterNumber);
//  return 1;
//}
//uint8_t GetConfigParameterDefaultValue(uint16_t parameterNumber)
//{
//  UNUSED(parameterNumber);
//  return 0;
//}
//
//// get the configuration parameter values
//uint32_t GetConfigParamValue(uint16_t parameterNumber)
//{
//  switch(parameterNumber)
//  {
//
//
//    default:
//      return 0xFE;
//    break;
//  }
//  return 0xFE;
//}



void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult)
{
  DPRINTF("\r\nTX CB for N %u", pTransmissionResult->nodeId);

  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
      zaf_event_distributor_enqueue_app_event(EVENT_APP_FINISH_EVENT_JOB);
  }
}

uint8_t cc_multilevel_switch_get_current_value(uint8_t endpoint)
{
  UNUSED(endpoint);
  return rotaryLevel;
}

uint8_t cc_multilevel_switch_get_target_value(uint8_t endpoint)
{
  UNUSED(endpoint);
  return rotaryLevel;
}

uint8_t cc_multilevel_switch_get_duration(uint8_t endpoint)
{
  UNUSED(endpoint);
  return 0;
}



void cc_multilevel_switch_start_level_change(uint8_t endpoint,
                                             bool up,
                                             bool ignore_start_level,
                                             uint8_t start_level,
                                             uint8_t duration)
{
  UNUSED(endpoint);
  UNUSED(up);
  UNUSED(ignore_start_level);
  UNUSED(start_level);
  UNUSED(duration);
}

void cc_multilevel_switch_stop_level_change(uint8_t endpoint)
{
  UNUSED(endpoint);
}

void cc_multilevel_switch_set(uint8_t value,uint8_t duration,uint8_t endpoint)
{
  UNUSED(duration);
  UNUSED(endpoint);
  UNUSED(value);
}

//
//void Send_Report_Timer_CB()
//{
//  Send_Report(ENDPOINT_ROOT);
//}
//
//
//
void LoadConfiguration()
{
 // ApplicationData = readAppData();
  DPRINT("LOAD CONFIGURATION\n");
 // loadStatusPowerLevel();
  return ;
}






void App_HW_Init_Timer_CB()
{
  DPRINT("App HW Init\n");
  Board_HW_Init();
  LoadConfiguration();
}

void
ZCB_TransmitCallback(TRANSMISSION_RESULT * pTransmissionResult)
{
  UNUSED(pTransmissionResult);
  DPRINTF("\r\nTX CB for N %d: %d", pTransmissionResult->nodeId, pTransmissionResult->status);
}

/**
 * Initiates a Central Scene Notification to the lifeline.
 * We don't care about the result, since we have to proceed no matter what.
 * Therefore a callback function is called in any case.
 * @param keyAttribute The key attribute in action.
 * @param sceneNumber The scene in action.
 */


