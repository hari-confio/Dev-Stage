
#include <ZW_classcmd.h>
#include <CC_Common.h>
#include <ZAF_types.h>
#include <ZW_TransportEndpoint.h>
#include <ZAF_TSE.h>



//received_frame_status_t CC_Configuration_handler_dali(
//  RECEIVE_OPTIONS_TYPE_EX * pRxOpt,
//  ZW_APPLICATION_TX_BUFFER * pCmd,
//  uint16_t cmdLength);

uint16_t GetConfigurationParameterValue(uint16_t parameterNumber);
uint32_t GetConfigurationParameterValue1(uint16_t parameterNumber);
void SetConfigurationParameterValue(uint16_t parameterNumber, uint32_t parameterValue);
void SetConfigurationParameterDefaultValue(uint16_t parameterNumber);
void switch_type(uint16_t endpoint);



typedef struct _APPLICATION_DATA1
{
  uint8_t config_addr;
}SApplicationData1;
extern SApplicationData1 ApplicationData1;
 //extern SApplicationData1 appData1;
void writeAppData(const SApplicationData1* pAppData);

 SApplicationData1 readAppData(void);

