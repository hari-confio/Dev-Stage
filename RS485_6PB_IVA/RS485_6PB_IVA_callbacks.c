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

#define LIGHT_ENDPOINT (1)

bool Network_open     = false;

uint8_t Button_1_Count = 0;




//EmberEventControl findingAndBindingEventControl;
EmberEventControl Switch_Counter_Control;
//EmberEventControl Network_Steering_Event_Control;
EmberEventControl appRepeat_NetworkSteeringEventControl;


//void findingAndBindingEventHandler()
//{
//  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
//    emberEventControlSetInactive(findingAndBindingEventControl);
//    emberAfCorePrintln("Find and bind target start: 0x%X",
//                       emberAfPluginFindAndBindTargetStart(LIGHT_ENDPOINT));
//  }
//}









/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
/*bool emberAfStackStatusCallback(EmberStatus status)
{
//  // Note, the ZLL state is automatically updated by the stack and the plugin.
//  if (status == EMBER_NETWORK_DOWN) {
//    halClearLed(COMMISSIONING_STATUS_LED);
//  } else if (status == EMBER_NETWORK_UP) {
//    halSetLed(COMMISSIONING_STATUS_LED);
//    emberEventControlSetActive(findingAndBindingEventControl);
//  }
//
//// This value is ignored by the framework.
  return false;
}*/

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup.
 * Any code that you would normally put into the top of the application's
 * main() routine should be put into this function.
        Note: No callback
 * in the Application Framework is associated with resource cleanup. If you
 * are implementing your application on a Unix host where resource cleanup is
 * a consideration, we expect that you will use the standard Posix system
 * calls, including the use of atexit() and handlers for signals such as
 * SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If you use the signal()
 * function to register your signal handler, please mind the returned value
 * which may be an Application Framework function. If the return value is
 * non-null, please make sure that you call the returned function from your
 * handler to avoid negating the resource cleanup of the Application Framework
 * itself.
 *
 */
//void emberAfMainInitCallback(void)
//{
//  //emberEventControlSetActive(commissioningLedEventControl);
//
////Saving the data from flash memory of the Boot Count Cluster
//  halCommonGetToken(&Boot_Count,TOKEN_BOOT_COUNT_1);
//   Boot_Count++;
//   int value[1]={Boot_Count};
//   emberAfWriteManufacturerSpecificServerAttribute(Endpoint_1, ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
//                                                   0x000D, EMBER_AF_MANUFACTURER_CODE, (uint8_t*)value, ZCL_INT32U_ATTRIBUTE_TYPE);
//   emberAfCorePrintln("====Boot_Count :  %d  ", Boot_Count);
//
//  emberEventControlSetActive(Button_Isr_Poll_Control);
//  Dim_Init();
//  interface_iic_init();
//  MCP23017_Init();
//  //app_apds9960_init();
//  Serial_Init();
////  GPIO_PinModeSet(DIM_PORT, DIM_PIN, gpioModeWiredAndPullUpFilter, ON);
//
////  GPIO_PinModeSet(PORT_SENSOR_INT, PIN_SENSOR_INT, gpioModeInputPullFilter, 0);
// // GPIO_ExtIntConfig(PORT_SENSOR_INT, PIN_SENSOR_INT, PIN_SENSOR_INT, true, true, true);
////  GPIOINT_CallbackRegister(PIN_SENSOR_INT, Sensor_interrupt_CB);
//
//}

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
//          emberEventControlSetActive(commissioningLedEventControl);
          emberEventControlSetDelayMS(appRepeat_NetworkSteeringEventControl, 10);
        }
      else
        {
          //emberEventControlSetInactive(commissioningLedEventControl);
          Network_open = false;
          count = 0;
        }
    }
  else
    {
      //emberEventControlSetInactive(commissioningLedEventControl);
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
//void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
//                                                 bool usedSecondaryChannels)
//{
//  emberAfCorePrintln("%p network %p: 0x%X",
//                     "Form distributed",
//                     "complete",
//                     EMBER_SUCCESS);
//}

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

///** @brief Server Attribute Changed
// *
// * On/off cluster, Server Attribute Changed
// *
// * @param endpoint Endpoint that is being initialized  Ver.: always
// * @param attributeId Attribute that changed  Ver.: always
// */
//void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
//                                                       EmberAfAttributeId attributeId)
//{
//  // When the on/off attribute changes, set the LED appropriately.  If an error
//  // occurs, ignore it because there's really nothing we can do.
// if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
//     {
//       bool onOff;
//
//       if (emberAfReadServerAttribute (endpoint,
//                                       ZCL_ON_OFF_CLUSTER_ID,
//                                       ZCL_ON_OFF_ATTRIBUTE_ID,
//                                       (uint8_t*) &onOff, sizeof(onOff))
//           == EMBER_ZCL_STATUS_SUCCESS )
//         {
//           emberAfCorePrintln("Endpoint = %d onOff = %d\n", endpoint, onOff);
//
//              switch (endpoint)
//                 {
//                 case 1:
//                    if(Bt6_Switch1 == 1)
//                      {
//                     if(onOff == ON)
//                          {
//                          GPIO_PinOutSet(GPIO_LED1_PORT, LED_1);
//                          }
//                     else if(onOff == OFF)
//                       {
//                         GPIO_PinOutClear(GPIO_LED1_PORT, LED_1);
//                       }
//                      }
//
//                   break;
//
//                 case 2:
//              if (onOff == ON)
//                {
//                  GPIO_PinOutSet (GPIO_LED2_PORT, LED_2);
//                }
//              else if (onOff == OFF)
//                {
//                  GPIO_PinOutClear (GPIO_LED2_PORT, LED_2);
//                }
//                   break;
//                 case 3:
//              if (onOff == ON)
//                {
//                  GPIO_PinOutSet (GPIO_LED3_PORT, LED_3);
//                }
//              else if (onOff == OFF)
//                {
//                  GPIO_PinOutClear (GPIO_LED3_PORT, LED_3);
//                }
//                   break;
//                 case 4:
//              if (onOff == ON)
//                {
//                  GPIO_PinOutSet (GPIO_LED4_PORT, LED_4);
//                }
//              else if (onOff == OFF)
//                {
//                  GPIO_PinOutClear (GPIO_LED4_PORT, LED_4);
//                }
//                   break;
//                  case 5:
//              if (onOff == ON)
//                {
//                  GPIO_PinOutSet (GPIO_LED5_PORT, LED_5);
//                }
//              else if (onOff == OFF)
//                {
//                  GPIO_PinOutClear (GPIO_LED5_PORT, LED_5);
//                }
//                    break;
//                  case 6:
//              if (onOff == ON)
//                {
//                  GPIO_PinOutSet (GPIO_LED6_PORT, LED_6);
//                }
//              else if (onOff == OFF)
//                {
//                  GPIO_PinOutClear (GPIO_LED6_PORT, LED_6);
//                }
//                    break;
//                  }
//         }
//     }
//}

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
//  if (button == Button_1 && state == Pressed)
//      {
//        emberEventControlSetDelayMS(Network_Steering_Event_Control , 10000);
//        emberEventControlSetDelayMS(Switch_Counter_Control, 500);
//        Button_1_Count++;
//        emberAfCorePrintln("************Button_1_Count = %d ", Button_1_Count);
//      }
//  if(button == Button_1 && state == Released)
//    {
//      emberEventControlSetInactive(Network_Steering_Event_Control);
//
////      emberEventControlSetInactive(commissioningLedEventControl);
//    }
//  emberAfCorePrintln("button = %d    state = %d",button,state);
}


bool emberAfCustomEndDeviceConfigurationClusterCommandOneCallback(uint8_t argOne)
{
  return false;
}
