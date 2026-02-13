/***************************************************************************//**
 * @file
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

#include "App_Scr/App_Main.h"

//#include EMBER_AF_API_NETWORK_CREATOR
//#include EMBER_AF_API_NETWORK_CREATOR_SECURITY
#include EMBER_AF_API_NETWORK_STEERING
#include EMBER_AF_API_FIND_AND_BIND_TARGET
//#include EMBER_AF_API_ZLL_PROFILE

bool Network_open     = false;
uint8_t Button_1_Count = 0;
EmberEventControl findingAndBindingEventControl;

//EmberEventControl Network_Steering_Event_Control;
EmberEventControl appRepeat_NetworkSteeringEventControl;


void findingAndBindingEventHandler()
{
//  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
//    emberEventControlSetInactive(findingAndBindingEventControl);
//    emberAfCorePrintln("Find and bind target start: 0x%X",
//                       emberAfPluginFindAndBindTargetStart(LIGHT_ENDPOINT));
//  }
}



/////////////////////////////////////////////////////////////////////////////////
/// Network Steering handler to Steer the network 4 times extra with 10MS delay
/////////////////////////////////////////////////////////////////////////////////
void appRepeat_NetworkSteeringEventHandler()
{
  emberEventControlSetInactive(appRepeat_NetworkSteeringEventControl);
  EmberStatus status = emberAfPluginNetworkSteeringStart();
  emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
}

/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
/////////////////////////////////////////////////////////////////////////////////
/// Network Steering handler to Steer the network 4 times extra with 10MS delay
/////////////////////////////////////////////////////////////////////////////////
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  uint8_t totalBeacons,
                                                  uint8_t joinAttempts,
                                                  uint8_t finalState)
{
  //In this callback calls network steering 4 times.
  static int count = 0;
  if(status == EMBER_NO_BEACONS)
  {
    if(count < 4)
    {
      count++;
      emberEventControlSetDelayMS(appRepeat_NetworkSteeringEventControl, 10);
    }
    else
    {
        Network_open = false;
        count = 0;
    }
    }
  else
  {
    Network_open = false;
    count = 0;
  }
}

/** @brief Complete
 *
 * This callback notifies the user that the network creation process has
 * completed successfully.
 *
 * @param network The network that the network creator plugin successfully
 * formed. Ver.: always
 * @param usedSecondaryChannels Whether or not the network creator wants to
 * form a network on the secondary channels Ver.: always
 */
void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                 bool usedSecondaryChannels)
{
  emberAfCorePrintln("%p network %p: 0x%X",
                     "Form distributed",
                     "complete",
                     EMBER_SUCCESS);
}

/** @brief On/off Cluster Server Post Init
 *
 * Following resolution of the On/Off state at startup for this endpoint, perform any
 * additional initialization needed; e.g., synchronize hardware state.
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfPluginOnOffClusterServerPostInitCallback(uint8_t endpoint)
{
  // At startup, trigger a read of the attribute and possibly a toggle of the
  // LED to make sure they are always in sync.
  emberAfOnOffClusterServerAttributeChangedCallback(endpoint,
                                                    ZCL_ON_OFF_ATTRIBUTE_ID);
}


/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
/////////////////////////////////////////////////////////////////////////////////
/// For the First button interrupt handler to switch modes of network
/////////////////////////////////////////////////////////////////////////////////
void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
{
  emberAfCorePrintln("emberAfHalButtonIsrCallback button %d, state %d",button, state);
}


bool emberAfCustomEndDeviceConfigurationClusterCommandOneCallback(uint8_t argOne)
{
  return false;
}
