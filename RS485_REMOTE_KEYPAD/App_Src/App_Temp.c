/*
 * App_Temp.c
 *
 *  Created on: Nov 18, 2022
 *      Author: Confio
 */

    #include "App_Src/App_Main.h"
    #include "App_Src/App_StatusUpdate.h"
    #include "App_Src/App_Button.h"
//   void appGPIOPollEventHandler()
//       {
//         emberEventControlSetInactive(appGPIOPollEventControl);
//         static bool B1 = true;
//         static bool B2 = true;
//         static bool B3 = true;
//         static bool B4 = true;
//         static bool B5 = true;
//         static bool B6 = true;
//         if((!GPIO_PinInGet(GPIO_BT1_PORT,GPIO_BT1_PIN)) == BUTTON_PRESSED && B1)
//           {
//             emberAfCorePrintln("B1 Pressed");
//             Button_count.Button1_Count++;
//             Button_Counter++;
//             emberEventControlSetDelayMS(appNetwork_Button_CounterEventControl,700); //  Button Inclusion/Exclusion handler
//             emberEventControlSetDelayMS(appButton1CounterEventControl, 700);
//             tensec_timer = true;
//             emberEventControlSetDelayMS(appTENsec_holdEventControl, 6000);
//             B1 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//         else if((!GPIO_PinInGet(GPIO_BT1_PORT,GPIO_BT1_PIN)) == BUTTON_RELEASED && !B1)
//           {
//             tensec_timer = false;
//             B1 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         if((!GPIO_PinInGet(GPIO_BT2_PORT,GPIO_BT2_PIN)) == BUTTON_PRESSED && B2)
//           {
//             emberAfCorePrintln("B2 Pressed");
//             Button_count.Button2_Count++;
//             emberEventControlSetDelayMS(appButton2CounterEventControl, 700);
//             B2 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//
//           }
//         else if((!GPIO_PinInGet(GPIO_BT2_PORT,GPIO_BT2_PIN)) == BUTTON_RELEASED  && !B2)
//           {
//             B2 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         if((!GPIO_PinInGet(GPIO_BT3_PORT,GPIO_BT3_PIN)) == BUTTON_PRESSED && B3)
//           {
//             emberAfCorePrintln("B3 Pressed");
//             Button_count.Button3_Count++;
//             emberEventControlSetDelayMS(appButton3CounterEventControl, 700);
//             B3 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//         else if((!GPIO_PinInGet(GPIO_BT3_PORT,GPIO_BT3_PIN)) == BUTTON_RELEASED  && !B3)
//           {
//             B3 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         if((!GPIO_PinInGet(GPIO_BT4_PORT,GPIO_BT4_PIN)) == BUTTON_PRESSED && B4)
//           {
//             emberAfCorePrintln("B4 Pressed");
//             Button_count.Button4_Count++;
//             emberEventControlSetDelayMS(appButton4CounterEventControl, 700);
//             B4 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//         else if((!GPIO_PinInGet(GPIO_BT4_PORT,GPIO_BT4_PIN)) == BUTTON_RELEASED  && !B4)
//           {
//             B4 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         if((!GPIO_PinInGet(GPIO_BT5_PORT,GPIO_BT5_PIN)) == BUTTON_PRESSED && B5)
//           {
//             emberAfCorePrintln("B5 Pressed");
//             Button_count.Button5_Count++;
//             emberEventControlSetDelayMS(appButton5CounterEventControl, 700);
//             B5 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//         else if((!GPIO_PinInGet(GPIO_BT5_PORT,GPIO_BT5_PIN)) == BUTTON_RELEASED  && !B5)
//           {
//             B5 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         if((!GPIO_PinInGet(GPIO_BT6_PORT,GPIO_BT6_PIN)) == BUTTON_PRESSED && B6)
//           {
//             emberAfCorePrintln("B6 Pressed");
//             Button_count.Button6_Count++;
//             emberEventControlSetDelayMS(appButton6CounterEventControl, 700);
//             B6 = false;
//             if(!Network_open) GPIO_PinOutClear(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//         else if((!GPIO_PinInGet(GPIO_BT6_PORT,GPIO_BT6_PIN)) == BUTTON_RELEASED  && !B6)
//           {
//             B6 = true;
//             if(!Network_open) GPIO_PinOutSet(GPIO_IDENTIFY_LED_PORT,GPIO_IDENTIFY_LED_PIN);// led
//           }
//
//
//         emberEventControlSetDelayMS(appGPIOPollEventControl,100);
//       }






//      void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
//    {
//                  emberAfCorePrintln("==============================button %d,  state %d",button,state);

//      if (!eliminate_Irs)
//          {
//            switch (button)
//            {
//              case BUTTON_1:
//                if(!state)
//                  GPIO_PinOutClear(GPIO_RL1_PORT,GPIO_RL1_PIN);
//
//                else
//                  {
//                    GPIO_PinOutSet(GPIO_RL1_PORT,GPIO_RL1_PIN);
//                    Button_count.Button1_Count++;
//                    Button_Counter++;
//                    emberEventControlSetDelayMS(appButtonCounterEventControl,700); //  Button Inclusion/Exclusion handler
//                    emberEventControlSetDelayMS(appButton1CounterEventControl, 700);
//                  }
//                break;
//
//              case BUTTON_2:
//                if(!state)
//                  GPIO_PinOutClear(GPIO_RL2_PORT,GPIO_RL2_PIN);
//
//                else
//                  {
//                    GPIO_PinOutSet(GPIO_RL2_PORT,GPIO_RL2_PIN);
//                    Button_count.Button2_Count++;
//                    emberEventControlSetDelayMS(appButton2CounterEventControl, 700);
//                  }
//                break;
//
//              case BUTTON_3:
//                if(!state)
//                  GPIO_PinOutClear(GPIO_RL3_PORT,GPIO_RL3_PIN);
//
//                else
//                  {
//                    GPIO_PinOutSet(GPIO_RL3_PORT,GPIO_RL3_PIN);
//                    Button_count.Button3_Count++;
//                    emberEventControlSetDelayMS(appButton3CounterEventControl, 700);
//                  }
//                break;
//
//              case BUTTON_4:
//                if(!state)
//                  GPIO_PinOutClear(GPIO_RL4_PORT,GPIO_RL4_PIN);
//
//                else
//                  {
//                    GPIO_PinOutSet(GPIO_RL4_PORT,GPIO_RL4_PIN);
//                    Button_count.Button4_Count++;
//                    emberEventControlSetDelayMS(appButton4CounterEventControl, 700);
//                  }
//                break;
//            }
//          }
//    }

