/**
 * Board initialization.
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */
#include <board_config.h>
#include <App_Serial.h>

#include <board_init.h>
#include <board.h>
#include <zw_region_config.h>
#include "zpal_radio.h"
#include "zpal_radio_utils.h"
#include "MfgTokens.h"
 // #include "MCP23017_I2C.h"
//#include "driver_interface.h"

void Board_Init(void)
{
  uint8_t rfRegion;
  uint8_t tokenRegion;

  Board_Initialize();
//  Serial_Init();
  rfRegion = zpal_radio_get_valid_region(ZW_REGION);

  ZW_GetMfgTokenDataCountryFreq(&tokenRegion);
  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED != zpal_radio_region_get_protocol_mode(tokenRegion))
  {
    rfRegion = tokenRegion;
  }

  BRD420xBoardInit(rfRegion);
  // interface_iic_init();
  Init_HW();
  Serail_Init();
  Serail_Init1();

  //MCP23017_Init();

}
