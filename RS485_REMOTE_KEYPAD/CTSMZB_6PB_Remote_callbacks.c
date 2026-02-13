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
#include "App_Src/App_Main.h"



#include EMBER_AF_API_NETWORK_STEERING
#include EMBER_AF_API_FIND_AND_BIND_TARGET

#define LIGHT_ENDPOINT (1)

EmberEventControl commissioningLedEventControl;
EmberEventControl findingAndBindingEventControl;
EmberEventControl appRepeat_NetworkSteeringEventControl;

void commissioningLedEventHandler(void)
{
  emberEventControlSetInactive(commissioningLedEventControl);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
  {
      GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
  }

  else
  {
      GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
  }

}

void findingAndBindingEventHandler()
{
    emberEventControlSetInactive(findingAndBindingEventControl);

//    if (emberAfNetworkState() == EMBER_NO_NETWORK)
//       {
//        emberAfPluginNetworkSteeringStart();
//       }
}







void appRepeat_NetworkSteeringEventHandler()
     {
      emberEventControlSetInactive(appRepeat_NetworkSteeringEventControl);
      EmberStatus status = emberAfPluginNetworkSteeringStart();
      emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
     }


 void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                   uint8_t totalBeacons,
                                                   uint8_t joinAttempts,
                                                   uint8_t finalState)
 {
   static int count=0;

   if (status == EMBER_NO_BEACONS)
     {
       if(count <4)
         {
          count++;
          emberEventControlSetDelayMS(appRepeat_NetworkSteeringEventControl, 10);
         }
       else
         {
           Network_open = false;
           count=0;
         }
     }
   else
     {
       Network_open = false;
       count=0;
     }
//   emberEventControlSetActive(commissioningLedEventControl);
   GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
   emberAfCorePrintln("************ Steering Complete");
 }









void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                 bool usedSecondaryChannels)
{
//  emberEventControlSetActive(commissioningLedEventControl);
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

/** @brief Server Attribute Changed
 *
 * On/off cluster, Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                       EmberAfAttributeId attributeId)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
  if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) {
    bool onOff;
    if (emberAfReadServerAttribute(endpoint,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (uint8_t *)&onOff,
                                   sizeof(onOff))
        == EMBER_ZCL_STATUS_SUCCESS) {
      if (onOff) {
//        halSetLed(ON_OFF_LIGHT_LED);
      } else {
//        halClearLed(ON_OFF_LIGHT_LED);
      }
    }
  }
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



void emberAfPluginEndDeviceSupportPollCompletedCallback(EmberStatus status)
{
  emberAfCorePrintln("emberAfPluginEndDeviceSupportPollCompletedCallback  %d", status);
}



