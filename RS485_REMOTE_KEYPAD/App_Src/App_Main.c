
/*
 * App_Main.c
 *
 *  Created on: Aug 24, 2022
 *      Author: Confio
 */

/*
 *
 *
 * Version update- Inlcuded multi-press of the button.
 *
 *
Button 1 (Long Press) to include to the network
  GREEN Led ON until connected to the network

Button 1 (13 Times press) to exclude from the network
  GREEN Led Blink one time when you resetting it.
*/


    #include "App_Main.h"
    #include "App_StatusUpdate.h"
    #include "App_Button.h"
    #include "app/framework/plugin-soc/idle-sleep/idle-sleep.h"
    #include "mx25flash_spi.h"
    #include "App_Button_Events.h"


    EmberEventControl appBroadcastSearchTypeEventControl;
    EmberEventControl appNetwork_Button_CounterEventControl; // Button Inclusion/Exclusion handler
    EmberEventControl appTENsec_holdEventControl;
    EmberEventControl sendExclusionDataControl;
    EmberEventControl appGPIOPollEventControl;
    EmberEventControl appButtonDebounceEventControl;
    EmberEventControl Button_Dimming_Control;

    ENDPOINTS ep;
    uint8_t Broadcast_seq = 1;
    uint16_t Boot_Count   = 0;
    uint8_t Button_Counter = 0;

    uint8_t Battery_Percent;
    uint32_t VBattery;

    bool tensec_timer = true;
    bool Button_Debounce = false;

    bool Network_open     = false;
    bool Battery_Per_Identifier = false;
    int dim_count = 0;
    uint8_t btn1Count = 0, btn2Count = 0, btn3Count = 0, dim_btn = 0;
    static void ADC_Input_Select(IADC_PosInput_t input);
    uint8_t MACID[8];

    int hold_btn = 0;
    bool btn1Hold = false, btn2Hold = false, btn3Hold = false,
        btn4Hold = false, btn5Hold = false, btn6Hold = false,
        btn1Press = false, btn2Press = false, btn3Press = false,
        btn4Press = false, btn5Press = false, btn6Press = false;
    int btn1Cnt = 0,btn2Cnt = 0,btn3Cnt = 0,btn4Cnt = 0,btn5Cnt = 0,btn6Cnt = 0;
    bool btn1LongHoldScheduled = false, btn2LongHoldScheduled = false, btn3LongHoldScheduled = false,
        btn4LongHoldScheduled = false, btn5LongHoldScheduled = false, btn6LongHoldScheduled = false;


    /**************************************************************************//**
     * @brief  IADC Pin Configurations.
     *****************************************************************************/
    static void ADC_Input_Select(IADC_PosInput_t input)
    {
      IADC_InitSingle_t initSingle   = IADC_INITSINGLE_DEFAULT;
      IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;
      singleInput.negInput = iadcNegInputGnd;
      singleInput.posInput = input; // the positive input (avdd) is scaled down by 4
      IADC_initSingle(IADC0, &initSingle, &singleInput);
    }


    /**************************************************************************//**
     * @brief  IADC Measuring the Battery voltage
     *****************************************************************************/
    uint32_t IADC_Vmeasure(void)
    {
      uint32_t sampleAVDD;


      ADC_Input_Select(iadcPosInputAvdd);
    //  singleInput.posInput   = IADC_INPUT_0_PORT_PIN;
      IADC_command(IADC0,iadcCmdStartSingle);

      while (IADC0->SINGLEFIFOSTAT == 0) ;
      sampleAVDD = IADC_pullSingleFifoData(IADC0);

      // Convert to mV (1.21V ref and 12 bit resolution).
        // Also we scale the sample up by 4 since the input was scaled down by 4
        sampleAVDD = ((sampleAVDD * 1210 ) / 4095) * 4 ;

        return sampleAVDD;
    }



    /**************************************************************************//**
     * @brief  IADC initialization
     *****************************************************************************/
    void IADC_Enable(void)
    {

      // Declare initialization structures
      IADC_Init_t init = IADC_INIT_DEFAULT;
      IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;

      // Initialize IADC
      IADC_init (IADC0, &init, &initAllConfigs);

    }

    /**************************************************************************//**
     * @brief  IADC Disabling
     *****************************************************************************/
    void IADC_Disable(void)
    {
      IADC_reset(IADC0);
         // Disable IADC clock
         CMU_ClockEnable(cmuClock_IADC0, false);
    }

    void Battery_Percent_Calculator(void)
    {
      IADC_Enable();
           VBattery = IADC_Vmeasure();
           emberAfCorePrintln("Battery Voltage : %dmV",VBattery);
           IADC_Disable();

           if(VBattery > 3000)
                   {
               Battery_Percent = 100;
                   }
                 else if(VBattery < 2400)
                   {
                     Battery_Percent = 0;
                   }
                 else
                   {
                     Battery_Percent = (((VBattery - Min_Battery_Volt) * 100)/(Max_Battery_Volt-Min_Battery_Volt));
                   }


           emberAfCorePrintln("Battery Percentage : %d'%'",Battery_Percent);
           Battery_Per_Identifier = true;
           if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
                       emberEventControlSetDelayMS(app_Endpoint1StatusUpdateEventControl,500);

    }
/******************************************************************************
 * Button Inclusion/Exclusion handler
 *****************************************************************************/

void sendExclusionDataHandler()
{
  emberEventControlSetInactive(sendExclusionDataControl);
  GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);

}

   void  appTENsec_holdEventHandler()
   {
     emberEventControlSetInactive(appTENsec_holdEventControl);
     if(tensec_timer)
       {
         if (emberAfNetworkState() == EMBER_NO_NETWORK)
           {
             Network_open = true;
//             emberEventControlSetActive(commissioningLedEventControl);
             EmberStatus status = emberAfPluginNetworkSteeringStart();
             emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);

             GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);

           }
         else if(emberAfNetworkState() == EMBER_JOINED_NETWORK)
           {
             emberFindAndRejoinNetworkWithReason(1,
                                                 0xFFFFFFFF,
                                                 EMBER_REJOIN_DUE_TO_END_DEVICE_REBOOT);

             emberEventControlSetActive(appBroadcastSearchTypeEventControl);
             emberEventControlSetDelayMinutes(appAnnouncementEventControl,5);



           }
       }
   }



   void appButtonDebounceEventHandler()
    {
      emberEventControlSetInactive(appButtonDebounceEventControl);
      Button_Debounce = false;
    }


   void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
    {
//      emberAfCorePrintln("Button Presed : %d, State  %d", button, state);
//      if (state == BUTTON_PRESSED && !Button_Debounce)
//        {
//          Button_Debounce = true;
//          emberEventControlSetDelayMS(appButtonDebounceEventControl, 200);
//              Button1_Event(state);
//          Button_Counter++;
//          emberEventControlSetDelayMS(appNetwork_Button_CounterEventControl, 700); //  Button Inclusion/Exclusion handler
//          tensec_timer = true;
//          emberEventControlSetDelayMS(appTENsec_holdEventControl, 6000);
//        }
//      else if(state == BUTTON_RELEASED)
//        {
//          tensec_timer = false;
//              Button1_Event(state);
//        }
//        }
    }


static uint8_t button_1_state = 0;
static uint8_t button_2_state = 0;
static uint8_t button_3_state = 0;
static uint8_t button_4_state = 0;
static uint8_t button_5_state = 0;
static uint8_t button_6_state = 0;


   void appGPIOPollEventHandler ()
    {




//      if ((!GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN)) == BUTTON_PRESSED )
//        {
////          Button_Debounce = true;
////          emberEventControlSetDelayMS(appButtonDebounceEventControl, 60);
//          state = TURN_ON;
////          Button1_Event(state);
//          emberAfCorePrintln("B1 Pressed");
////          emberEventControlSetDelayMS(appButton1CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button1_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint1StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN)) == BUTTON_RELEASED )
//        {
////          Button_Debounce = true;
////          emberEventControlSetDelayMS(appButtonDebounceEventControl, 50);
//          state = TURN_OFF;
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//                      Button1_Event(state);
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }
//
//      if ((!GPIO_PinInGet (GPIO_BT2_PORT, GPIO_BT2_PIN)) == BUTTON_PRESSED)
//        {
//          state = TURN_ON;
//          emberAfCorePrintln("B2 Pressed");
////          emberEventControlSetDelayMS(appButton2CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button2_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint2StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT2_PORT, GPIO_BT2_PIN)) == BUTTON_RELEASED)
//        {
//          state = TURN_OFF;
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//              Button2_Event(state);
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }
//
//      if ((!GPIO_PinInGet (GPIO_BT3_PORT, GPIO_BT3_PIN)) == BUTTON_PRESSED )
//        {
//          state = TURN_ON;
//          emberAfCorePrintln("B3 Pressed");
////          emberEventControlSetDelayMS(appButton3CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button3_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint3StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT3_PORT, GPIO_BT3_PIN)) == BUTTON_RELEASED)
//        {
//          state = TURN_OFF;
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//              Button3_Event(state);
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }
//
//      if ((!GPIO_PinInGet (GPIO_BT4_PORT, GPIO_BT4_PIN)) == BUTTON_PRESSED )
//        {
//          state = TURN_ON;
//          emberAfCorePrintln("B4 Pressed");
////          emberEventControlSetDelayMS(appButton4CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button4_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint4StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT4_PORT, GPIO_BT4_PIN)) == BUTTON_RELEASED)
//        {
//          state = TURN_OFF;
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//              Button4_Event(state);
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }
//
//      if ((!GPIO_PinInGet (GPIO_BT5_PORT, GPIO_BT5_PIN)) == BUTTON_PRESSED)
//        {
//          state = TURN_ON;
//          emberAfCorePrintln("B5 Pressed");
////          emberEventControlSetDelayMS(appButton5CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button5_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint5StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT5_PORT, GPIO_BT5_PIN)) == BUTTON_RELEASED)
//        {
//          state = TURN_OFF;
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//              Button5_Event(state);
//
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }
//
//      if ((!GPIO_PinInGet (GPIO_BT6_PORT, GPIO_BT6_PIN)) == BUTTON_PRESSED)
//        {
//          state = TURN_ON;
//          emberAfCorePrintln("B6 Pressed");
////          emberEventControlSetDelayMS(appButton6CounterEventControl, 700);
//          if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//            Button6_Event(state);
////            emberEventControlSetDelayMS(app_Endpoint6StatusUpdateEventControl,200);
////          if (!Network_open)
////            GPIO_PinOutClear (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//
//
//        }
//      else if ((!GPIO_PinInGet (GPIO_BT6_PORT, GPIO_BT6_PIN)) == BUTTON_RELEASED)
//        {
//          state = TURN_OFF;
//           if(emberAfNetworkState () == EMBER_JOINED_NETWORK)
//               Button6_Event(state);
////          if (!Network_open)
////            GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN); // led
//        }

    }


   int ButtonGPIO_Init(void)
   {
      CMU_ClockEnable(cmuClock_GPIO, true);  //no need of enabling clk in EFR32MG21

      GPIO_PinModeSet(gpioPortC, 0, gpioModePushPull, 0);

       //BUTTON_1
      GPIO_PinModeSet(GPIO_BT1_PORT, GPIO_BT1_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT1_PORT, GPIO_BT1_PIN, GPIO_BT1_PIN,true,true,true);

      //BUTTON_2
      GPIO_PinModeSet(GPIO_BT2_PORT, GPIO_BT2_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT2_PORT, GPIO_BT2_PIN, GPIO_BT2_PIN,true,true,true);

      //BUTTON_3
      GPIO_PinModeSet(GPIO_BT3_PORT, GPIO_BT3_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT3_PORT, GPIO_BT3_PIN, GPIO_BT3_PIN,true,true,true);

      //BUTTON_4
      GPIO_PinModeSet(GPIO_BT4_PORT, GPIO_BT4_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT4_PORT, GPIO_BT4_PIN, GPIO_BT4_PIN,true,true,true);

      //BUTTON_5
      GPIO_PinModeSet(GPIO_BT5_PORT, GPIO_BT5_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT5_PORT, GPIO_BT5_PIN, GPIO_BT5_PIN,true,true,true);

      //BUTTON_6
      GPIO_PinModeSet(GPIO_BT6_PORT, GPIO_BT6_PIN, HAL_GPIO_MODE_INPUT_PULL_FILTER, HAL_GPIO_DOUT_HIGH);
      GPIO_ExtIntConfig(GPIO_BT6_PORT, GPIO_BT6_PIN, GPIO_BT6_PIN,true,true,true);


      // Enable EVEN interrupt to catch button press that changes slew rate
      NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
      NVIC_EnableIRQ(GPIO_EVEN_IRQn);

      // Enable ODD interrupt to catch button press that changes slew rate
      NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
      NVIC_EnableIRQ(GPIO_ODD_IRQn);


      button_1_state = GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN);
      button_2_state = GPIO_PinInGet (GPIO_BT2_PORT, GPIO_BT2_PIN);
      button_3_state = GPIO_PinInGet (GPIO_BT3_PORT, GPIO_BT3_PIN);
      button_4_state = GPIO_PinInGet (GPIO_BT4_PORT, GPIO_BT4_PIN);
      button_5_state = GPIO_PinInGet (GPIO_BT5_PORT, GPIO_BT5_PIN);
      button_6_state = GPIO_PinInGet (GPIO_BT6_PORT, GPIO_BT6_PIN);


      return 0;
   }



    void emberAfMainInitCallback(void)
    {

     emberEventControlSetActive(commissioningLedEventControl);
//     GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led


     halCommonGetToken(&Boot_Count,TOKEN_BOOT_COUNT_1);
     Boot_Count++;
     int value[1]={Boot_Count};
     emberAfWriteManufacturerSpecificServerAttribute(Endpoint_1, ZCL_CUSTOM_END_DEVICE_CONFIGURATION_CLUSTER_ID,
                                                     0x000D, EMBER_AF_MANUFACTURER_CODE, (uint8_t*)value, ZCL_INT32U_ATTRIBUTE_TYPE);

     emberAfCorePrintln("====Boot_Count :  %d  ", Boot_Count);
     emberAfCorePrintln("====Current Version:  %d", EMBER_AF_PLUGIN_OTA_CLIENT_POLICY_FIRMWARE_VERSION);
     emberAfCorePrintln("====ImageTypeId    :  0x%2X", EMBER_AF_PLUGIN_OTA_CLIENT_POLICY_IMAGE_TYPE_ID);
     emberAfCorePrintln("====ManufacturerId :  0x%2X", EMBER_AF_MANUFACTURER_CODE);



     ButtonGPIO_Init();

     EmberEUI64 macId;
     emberAfGetEui64(macId);  // Get the MAC ID
     for (int i = 0; i < 8; i++) {
         MACID[i] = macId[7 - i];
     }

     emberAfCorePrint("MAC Bytes: ");
     for (int i = 0; i < 8; i++) {
         emberAfCorePrint("%X", MACID[i]);

//         btn1Buff[i] = MACID[i];
//         btn2Buff[i] = MACID[i];
//         btn3Buff[i] = MACID[i];
//         btn4Buff[i] = MACID[i];
//         btn5Buff[i] = MACID[i];
//         btn6Buff[i] = MACID[i];
   //      emberAfCorePrint("%X", btn6Buff[i]);
     }
     emberAfCorePrintln("");

//
////       Power-down the radio board SPI flash
//       FlashStatus status;
//       MX25_init ();
//       MX25_RSTEN ();
//       MX25_RST (&status);
//       MX25_DP ();
//       MX25_deinit ();

       halInternalDisableWatchDog (MICRO_DISABLE_WATCH_DOG_KEY);

    }


    void appBroadcastSearchTypeEventHandler()
    {
      emberEventControlSetInactive(appBroadcastSearchTypeEventControl);

      uint8_t Radio_channel = emberAfGetRadioChannel();
//    emberAfCorePrintln("======Radio_channel=====%d",Radio_channel);

      EmberApsFrame emAfCommandApsFrame;
      emAfCommandApsFrame.profileId = 0xC25D;
      emAfCommandApsFrame.clusterId = 0x0001;
      emAfCommandApsFrame.sourceEndpoint = 0xC4;
      emAfCommandApsFrame.destinationEndpoint = 0xC4;
      emAfCommandApsFrame.options = EMBER_APS_OPTION_SOURCE_EUI64 ;

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

                        // c4 6button
//                        0X1A,
//                        0x63, 0x34, 0x3A, 0x63, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x34, 0x5F, 0x6B, 0x70, 0x3A,
//                        0X43, 0x34, 0x2D, 0x4B, 0x43, 0x31, 0x32, 0x30, 0x32, 0x37, 0x37,

                        //insona
//                        0x14,
//                        0x69, 0x6e, 0x53, 0x6F, 0x6E, 0x61, 0x3A, 0x49, 0x4E, 0x2D, 0x43, 0x30, 0x31, 0x2D, 0x4D, 0x46,
//                        0x50, 0x2D, 0x53, 0x36,
//                          0x0C,                       // Length
//                          0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x43, 0x54, 0x36, 0x50, 0x42, //(Confio:CT6PB)
                        0x12,                       // Length
                        0x43, 0x6F, 0x6E, 0x66, 0x69, 0x6F, 0x3A, 0x52, 0x53, 0x34, 0x38, 0x35, 0x52, 0x45, 0x4D, 0x4F, 0x54, 0x45, //(Confio:RS485REMOTE)

                        0x0C, 0x00,  //MESH_CHANNEL
                        0x20,
                        Radio_channel,
                        };

      uint16_t dataLength = sizeof(data);
      EmberStatus status = emberAfSendBroadcastWithCallback(0xFFFC,
                                                             &emAfCommandApsFrame,
                                                              dataLength,
                                                              data,
                                                              NULL);
      emberAfCorePrintln("=====Broadcast Status  :  0x%02X\n",status);
      Broadcast_seq++;
    }




    bool emberAfStackStatusCallback(EmberStatus status)
    {

      emberAfCorePrintln("emberAfStackStatusCallback  0x%02X\n",status);
      emberAfCorePrintln("emberAfNetworkState()  0x%02X\n",emberAfNetworkState());

      if (emberAfNetworkState() == EMBER_JOINED_NETWORK)
       {
         Network_open = false;
       //  emberEventControlSetDelayMS(appBroadcastSearchTypeEventControl,500);
         emberEventControlSetDelayMS(appAnnouncementEventControl,2000);

         if(GPIO_PinOutGet(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN))
           {
             GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
           }
         emberEventControlSetActive(commissioningLedEventControl);

         emberAfCorePrintln("=====App EMBER_JOINED_NETWORK  emberAfPollControlClusterServerInitCallback :  0x%02X\n",status);
         emberAfPollControlClusterServerInitCallback(1);
        // set_Button_event(1, 4);
       }

      if (status == EMBER_NETWORK_DOWN)
       {
          if (emberAfNetworkState() == EMBER_JOINED_NETWORK_NO_PARENT)
            {
              GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
            }
          emberAfCorePrintln("=====App EMBER_NETWORK_DOWN  emberFindAndRejoinNetworkWithReason :  0x%02X\n",status);
       }

      if(status == EMBER_JOIN_FAILED)
        {
          GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
        }

//      emberEventControlSetActive(commissioningLedEventControl);

      return false;
    }






    uint8_t active_button = 0;
    uint8_t active_count = 0;

//    void appNetwork_Button_CounterEventHandler()
//    {
//      emberEventControlSetInactive(appNetwork_Button_CounterEventControl);
//      emberAfCorePrintln("Actvie Button  %d %d       ",active_button, active_count);
//
//      if(active_button == 1 && active_count == 4)
//        {
//          emberAfCorePrintln("emberAfNetworkState()   %d       ",emberAfNetworkState());
//          if (emberAfNetworkState() == EMBER_NO_NETWORK)
//           {
//             Network_open = true;
//             EmberStatus status = emberAfPluginNetworkSteeringStart();
//             emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
//             GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
//           }
//         else if(emberAfNetworkState() == EMBER_JOINED_NETWORK)
//           {
//             emberFindAndRejoinNetworkWithReason(1,
//                                                 0xFFFFFFFF,
//                                                 EMBER_REJOIN_DUE_TO_END_DEVICE_REBOOT);
//
//             emberEventControlSetActive(appBroadcastSearchTypeEventControl);
//             emberEventControlSetDelayMinutes(appAnnouncementEventControl,5);
//
//           }
//        }
//
//      if(active_button == 1 && active_count >= 13)
//        {
//          EmberStatus status = emberLeaveNetwork();
//          emberAfCorePrintln("Network Leave: 0x%X",status);
//          GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
//          emberEventControlSetDelayMS(commissioningLedEventControl, 1000);
//        }
//
//
//      set_Button_event(active_button, active_count);
//
//      active_count = 0;
//      active_button = 0;
//
//      Battery_Percent_Calculator();
//    }


    void appNetwork_Button_CounterEventHandler()
    {
      emberEventControlSetInactive(appNetwork_Button_CounterEventControl);
      emberAfCorePrintln("Actvie Button  %d %d       ",active_button, active_count);

      if(active_button == 2 && active_count == 4)
        {
          emberAfCorePrintln("emberAfNetworkState()   %d       ",emberAfNetworkState());
          if (emberAfNetworkState() == EMBER_NO_NETWORK)
           {
             Network_open = true;
             EmberStatus status = emberAfPluginNetworkSteeringStart();
             emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
             GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
           }
         else if(emberAfNetworkState() == EMBER_JOINED_NETWORK)
           {
             emberFindAndRejoinNetworkWithReason(1,
                                                 0xFFFFFFFF,
                                                 EMBER_REJOIN_DUE_TO_END_DEVICE_REBOOT);

             emberEventControlSetActive(appBroadcastSearchTypeEventControl);
             emberEventControlSetDelayMinutes(appAnnouncementEventControl,5);

           }
        }

      if(active_button == 2 && active_count >= 13)
        {
//          set_Button_event(active_button, active_count);
//      //    emberEventControlSetDelayMS(sendExclusionDataControl, 1000);
          EmberStatus status = emberLeaveNetwork();
          emberAfCorePrintln("Network Leave: 0x%X",status);
          GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
          emberEventControlSetDelayMS(commissioningLedEventControl, 1000);
        }

      else if (active_button == 1 || active_button == 50)
      {
          if (active_button == 1 && active_count == 1)
          {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

          }
          else if (active_count == 2)
          {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

          }
          else if (active_button == 50) // 50 indicates button long hold action
          {
              emberAfCorePrintln("---> Button %d Long Held\n", 1);

          }


      }

      else if(active_button == 2 || active_button == 60)
        {
          if(active_button == 2 && active_count == 1)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_count == 2)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_button == 60)
            {
              emberAfCorePrintln("---> Button %d Long Held\n", 2);

            }
        }

      else if(active_button == 3 || active_button == 70)
        {
          if(active_button == 3 && active_count == 1)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_count == 2)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_button == 70)
            {
              emberAfCorePrintln("---> Button %d Long Held\n", 3);

            }
        }
      else if(active_button == 4 || active_button == 80)
        {
          if(active_button == 4 && active_count == 1)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_count == 2)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_button == 80)
            {
              emberAfCorePrintln("---> Button %d Long Held\n", 4);

            }
        }
      else if(active_button == 5 || active_button == 90)
        {
          if(active_button == 5 && active_count == 1)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_count == 2)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_button == 90)
            {
              emberAfCorePrintln("---> Button %d Long Held\n", 5);

            }
        }
      else if(active_button == 6 || active_button == 100)
        {
          if(active_button == 6 && active_count == 1)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_count == 2)
            {
              emberAfCorePrintln("---> Button %d Pressed %d time(s)\n", active_button, active_count);

            }
          else if(active_button == 100)
            {
              emberAfCorePrintln("---> Button %d Long Held\n", 6);

            }
        }

      //
      if(active_button == 1 && (active_count == 4 || active_count >= 13))
        {
          GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
          emberEventControlSetDelayMS(sendExclusionDataControl, 1000);
        }
      set_Button_event(active_button, active_count);

      active_count = 0;
      active_button = 0;

      Battery_Percent_Calculator();
    }

    void button_presses(uint8_t button)
    {
      static uint8_t prv_button = 0;
      active_button = button;
      active_count++;

      if(prv_button != active_button)
        {
          active_count=1;
          prv_button = active_button;
        }
      emberEventControlSetDelayMS(appNetwork_Button_CounterEventControl, 400);
    }

    void create_frame(int button, int action_type)
    {
    //
    }
    void Button_Dimming_Handler()
    {
        emberEventControlSetInactive(Button_Dimming_Control); // Stop repeated calls

        if (hold_btn == 1 && !btn1Hold)  // Only process long hold once per press
        {
            btn1Hold = true;
            emberAfCorePrintln("Button1 Long Hold");
            button_presses(50);  // Call long hold action
        }
        else if (hold_btn == 2 && !btn2Hold)
        {
            btn2Hold = true;
            emberAfCorePrintln("Button2 Long Hold");
            button_presses(60);
        }
        else if (hold_btn == 3 && !btn3Hold)
        {
            btn3Hold = true;
            emberAfCorePrintln("Button3 Long Hold");
            button_presses(70);
        }
        else if (hold_btn == 4 && !btn4Hold)
        {
            btn4Hold = true;
            emberAfCorePrintln("Button4 Long Hold");
            button_presses(80);
        }
        else if (hold_btn == 5 && !btn5Hold)
        {
            btn5Hold = true;
            emberAfCorePrintln("Button5 Long Hold");
            button_presses(90);
        }
        else if (hold_btn == 6 && !btn6Hold)
        {
            btn6Hold = true;
            emberAfCorePrintln("Button6 Long Hold");
            button_presses(100);
        }
    }


   void emberAfPluginIdleSleepWakeUpCallback(uint32_t durationMs)
    {
      //emberAfCorePrintln("emberAfPluginIdleSleepWakeUpCallback durationMs %d",durationMs);
      //emberEventControlSetActive(appGPIOPollEventControl);
      //emberAfCorePrintln(" ====appGPIOPollEventHandler");
//       uint8_t state;
       emberEventControlSetInactive(appGPIOPollEventControl);

//       if(button_1_state != GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN))
//         {
//           button_1_state = GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN);
//           if(button_1_state == 0)
//             {
//               emberAfCorePrintln("B1 Pressed");
////               set_Button_event(1, 129);
////               button_presses(1);
//               emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
//               if(!btn1Hold)
//                 {
//                   btn1Press = true;
//
//                 }
//
//               dim_btn = 1;
//             }
//           else
//             {
//               emberAfCorePrintln("B1 Released");
////               set_Button_event(1, 130);
//               if(!btn1Hold && btn1Press)
//                 {
//                   set_Button_event(1, 129);
//                            button_presses(1);
//                            set_Button_event(1, 130);
//
//                 }
//               btn1Hold = false;
//              // set_led(1, 0);
//               emberEventControlSetInactive(Button_Dimming_Control);
//             }
//         }

       if(button_1_state != GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN))
         {
           button_1_state = GPIO_PinInGet (GPIO_BT1_PORT, GPIO_BT1_PIN);
           if(button_1_state == 0)
             {
               if (!btn1LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn1LongHoldScheduled = true;
               }

               if (!btn1Hold)
               {
                   btn1Press = true;
               }
               emberAfCorePrintln("Button1 Pressed");
               hold_btn = 1;
             }
           else
             {
               emberAfCorePrintln("Button1 Released");
               if (!btn1Hold && btn1Press)  // Short press action
               {
                   button_presses(1);

                   if (btn1Cnt == 0)
                   {
                       btn1Cnt = 1;
      //                 set_led(1, 1);
                   }
                   else if (btn1Cnt == 1)
                   {
                       btn1Cnt = 0;
      //                 set_led(1, 0);
                   }
               }

               // Reset flags only after release
               btn1Hold = false;
               btn1LongHoldScheduled = false; // Allow new long hold event for next press
               emberEventControlSetInactive(Button_Dimming_Control);
             }
         }
       if(button_2_state != GPIO_PinInGet (GPIO_BT2_PORT, GPIO_BT2_PIN))
         {
           button_2_state = GPIO_PinInGet (GPIO_BT2_PORT, GPIO_BT2_PIN);
           if(button_2_state == 0)
             {
               if (!btn2LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn2LongHoldScheduled = true;
               }
               if(!btn2Hold)
                 {
                   btn2Press = true;
                 }
               emberAfCorePrintln("Button2 Pressed");
                hold_btn = 2;

             }
           else
             {
               emberAfCorePrintln("Button2 Released");

               if(!btn2Hold && btn2Press)
                 {
                   button_presses(2);
                   if(btn2Cnt == 0)
                     {
                       btn2Cnt = 1;
                    //   set_led(2, 1);

                     }
                   else if(btn2Cnt == 1)
                     {
                       btn2Cnt = 0;
                     //  set_led(2,0);

                     }
                 }
               btn2Hold = false;
               btn2LongHoldScheduled = false;
               emberEventControlSetInactive(Button_Dimming_Control);
             }


         }

       if(button_3_state != GPIO_PinInGet (GPIO_BT3_PORT, GPIO_BT3_PIN))
         {
           button_3_state = GPIO_PinInGet (GPIO_BT3_PORT, GPIO_BT3_PIN);
           if(button_3_state == 0)
             {
               emberAfCorePrintln("Button3 Pressed");
               if (!btn3LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn3LongHoldScheduled = true;
               }
               if(!btn3Hold)
                 {
                   btn3Press = true;
                 }

               hold_btn = 3;

               emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
             }
           else
             {
               emberAfCorePrintln("Button3 Released");
               if(!btn3Hold && btn3Press)
                  {

               button_presses(3);
               if(btn3Cnt == 0)
                 {
                   btn3Cnt = 1;
                  // set_led(3, 1);

                 }
               else if(btn3Cnt == 1)
                 {
                   btn3Cnt = 0;
                  // set_led(3,0);
                 }
                  }
               btn3Hold = false;
               btn3LongHoldScheduled = false;
               emberEventControlSetInactive(Button_Dimming_Control);
             }
         }

       if(button_4_state != GPIO_PinInGet (GPIO_BT4_PORT, GPIO_BT4_PIN))
         {
           button_4_state = GPIO_PinInGet (GPIO_BT4_PORT, GPIO_BT4_PIN);
           if(button_4_state == 0)
             {
               emberAfCorePrintln("Button4 Pressed");
               if (!btn4LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn4LongHoldScheduled = true;
               }
               if(!btn4Hold)
                 {
                   btn4Press = true;
                 }

               hold_btn = 4;

               emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
             }
           else
             {
               emberAfCorePrintln("Button4 Released");
               if(!btn4Hold && btn4Press)
                     {
               button_presses(4);
                  if(btn4Cnt == 0)
                    {
                      btn4Cnt = 1;

                    //  set_led(4, 1);
                    }
                  else if(btn4Cnt == 1)
                    {
                      btn4Cnt = 0;
                    //  set_led(4,0);

                    }
                     }
                  btn4Hold = false;
                  btn4LongHoldScheduled = false;
               emberEventControlSetInactive(Button_Dimming_Control);
             }
         }

       if(button_5_state != GPIO_PinInGet (GPIO_BT5_PORT, GPIO_BT5_PIN))
         {
           button_5_state = GPIO_PinInGet (GPIO_BT5_PORT, GPIO_BT5_PIN);
           if(button_5_state == 0)
             {
               emberAfCorePrintln("Button5 Pressed");
               if (!btn5LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn5LongHoldScheduled = true;
               }
               hold_btn = 5;
               if(!btn5Hold)
                   {
                     btn5Press = true;
                   }

               emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
             }
           else
             {
               emberAfCorePrintln("Button5 Released");
               if(!btn5Hold && btn5Press)
             {
               button_presses(5);
               if(btn5Cnt == 0)
                 {
                   btn5Cnt = 1;
                  // set_led(5, 1);
                 }
               else if(btn5Cnt == 1)
                 {
                   btn5Cnt = 0;
                 //  set_led(5,0);

                 }
             }
               btn5Hold = false;
               btn5LongHoldScheduled = false;
               emberEventControlSetInactive(Button_Dimming_Control);
             }
         }

       if(button_6_state != GPIO_PinInGet (GPIO_BT6_PORT, GPIO_BT6_PIN))
         {
           button_6_state = GPIO_PinInGet (GPIO_BT6_PORT, GPIO_BT6_PIN);
           if(button_6_state == 0)
             {
               emberAfCorePrintln("Button6 Pressed");
               if (!btn6LongHoldScheduled) // Ensure event is scheduled only once
               {
                   emberEventControlSetDelayMS(Button_Dimming_Control, 1000);
                   btn6LongHoldScheduled = true;
               }
               if(!btn6Hold)
                    {
                      btn6Press = true;
                    }


               hold_btn = 6;


               emberEventControlSetDelayMS(Button_Dimming_Control, 1000);

             }
           else
             {
               emberAfCorePrintln("Button6 Released");
               if(!btn6Hold && btn6Press)
                 {
               button_presses(6);
               if(btn6Cnt == 0)
                 {
                   btn6Cnt = 1;
                  // set_led(6, 1);
                 }
               else if(btn6Cnt == 1)
                 {
                   btn6Cnt = 0;
                 //  set_led(6,0);

                 }
              }
                btn6Hold = false;
                btn6LongHoldScheduled = false;
                emberEventControlSetInactive(Button_Dimming_Control);
             }

         }
    }


    /** @brief CheckInTimeout
     *
     * This function is called by the Poll Control Server plugin after a threshold value of poll control
     * check in messages are sent to a trust center and no responses are received. The existence of this
     * callback provides an opportunity for the application to implement its own rejoin algorithm or logic.
     */
    void emberAfPluginPollControlServerCheckInTimeoutCallback(void)
    {
      if (emberAfNetworkState() == EMBER_NO_NETWORK)
                 {
                   Network_open = true;
                   EmberStatus status = emberAfPluginNetworkSteeringStart();
                   emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
                   GPIO_PinOutSet (GPIO_IDENTIFY_LED_PORT, GPIO_IDENTIFY_LED_PIN);
                 }
               else if(emberAfNetworkState() == EMBER_JOINED_NETWORK)
                 {
                   emberFindAndRejoinNetworkWithReason(1,
                                                       0xFFFFFFFF,
                                                       EMBER_REJOIN_DUE_TO_END_DEVICE_REBOOT);

                   emberEventControlSetActive(appBroadcastSearchTypeEventControl);
                   emberEventControlSetDelayMinutes(appAnnouncementEventControl,5);

                 }
    }

    boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
    {
      emberAfCorePrintln("--->emberAfPreCommandReceivedCallback");
      int attributeId = cmd->buffer[3];
      if (cmd->apsFrame->clusterId == 0x0001 && cmd->commandId == 0)
         {
          if(attributeId ==  3) // replay to controller announcing attributes
            {
              emberEventControlSetActive(appAnnouncementEventControl);
            }
         }
      return false;
    }

    EmberAfStatus emberAfCustomEndDeviceConfigurationClusterServerPreAttributeChangedCallback(int8u endpoint,
                                                                                              EmberAfAttributeId attributeId,
                                                                                              EmberAfAttributeType attributeType,
                                                                                              int8u size,
                                                                                              int8u *value)
    {
      emberAfCorePrintln(" ===========in callback = 0x%02X ",attributeId);
      return 1;
    }

    boolean emberAfCustomEndDeviceConfigurationClusterCommandOneCallback(int8u argOne)
    {
//MACID[0],
    }

