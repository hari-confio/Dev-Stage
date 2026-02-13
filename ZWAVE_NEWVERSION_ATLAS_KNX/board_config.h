
#ifndef SRC_BOARD_CONFIG_H_
#define SRC_BOARD_CONFIG_H_


#include <stdint.h>

#include "em_gpio.h"
#include "ev_man.h"


//Port A
#define UART0_TX_PORT		gpioPortA
#define UART0_TX_PIN		0

#define UART0_RX_PORT 		gpioPortA
#define UART0_RX_PIN		1

#define PORT_A2 			gpioPortA
#define PIN_A2				2

#define PORT_A3 			gpioPortA
#define PIN_A3				3

#define PORT_A4 			gpioPortA
#define PIN_A4				4

#define PORT_A5 			gpioPortA
#define PIN_A5				5


//Port B
#define PORT_B11 			gpioPortB
#define PIN_B11				11

#define PORT_B12 			gpioPortB
#define PIN_B12				12

#define PORT_B13 			gpioPortB
#define PIN_B13				13

#define PORT_B14 			gpioPortB
#define PIN_B14				14

#define PORT_B15 			gpioPortB
#define PIN_B15				15



//Port C
#define PORT_C6 			gpioPortC
#define PIN_C6				6

#define PORT_C7 			gpioPortC
#define PIN_C7				7

#define PORT_C8 			gpioPortC
#define PIN_C8				8

#define PORT_C9 			gpioPortC
#define PIN_C9				9

#define PORT_C10 			gpioPortC
#define PIN_C10				10

#define PORT_C11 			gpioPortC
#define PIN_C11				11



//Port D
#define PORT_D9 			gpioPortD
#define PIN_D9				9

#define PORT_D10 			gpioPortD
#define PIN_D10				10

#define PORT_D11 			gpioPortD
#define PIN_D11				11

#define PORT_D12 			gpioPortD
#define PIN_D12				12

#define LEARN_RESET_BTN_PORT 			gpioPortD
#define LEARN_RESET_BTN_PIN				13

#define PORT_D14 			gpioPortD
#define PIN_D14				14

#define PORT_D15 			gpioPortD
#define PIN_D15				15



//Port F
#define PORT_F0 			gpioPortF
#define PIN_F0				0

#define PORT_F1 			gpioPortF
#define PIN_F1				1

#define PORT_F2 			gpioPortF
#define PIN_F2				2

#define PORT_F3				gpioPortF
#define PIN_F3				3

#define PORT_F4 			gpioPortF
#define PIN_F4				4

#define PORT_F5 			gpioPortF
#define PIN_F5				5

#define PORT_F6 			gpioPortF
#define PIN_F6				6

#define PORT_F7 			gpioPortF
#define PIN_F7				7


/*
 *  @brief Configure Button Mode
 *  */
#define InputMode			gpioModeInputPullFilter

/**
 *  @brief Configure LED Mode
 */
#define OutputMode			gpioModePushPull

#define true 			1
#define false			0


#define ON				0xFF
#define OFF				0x00


#define SW_OFF				1
#define SW_ON				0
//#define DEFINE_EVENT_KEY_NBR 0x40
//#define DEFINE_EVENT_APP_NBR 127//EVENTS ARE MISMATCHING AS I HAVE ADDED THE EVENTS MORE, SO CHANGED FROM 0X70


typedef enum APP_BUTTON_EVENT_
{
  APP_EVENT_PB_DOWN = DEFINE_EVENT_KEY_NBR,
  APP_EVENT_PB_UP,
  APP_EVENT_PB_SHORT_PRESS,
  APP_EVENT_PB_HOLD,
  APP_EVENT_PB_LONG_PRESS,

  APP_EVENT_SW1_TRIPLE_PRESS,
  APP_EVENT_SW1_15_TIMES_PRESS,

  APP_EVENT_SCENE_1,
  APP_EVENT_SCENE_2,
  APP_EVENT_SCENE_3,
  APP_EVENT_SCENE_4,
  APP_EVENT_SCENE_5,
  APP_EVENT_SCENE_6,
  APP_EVENT_PROCESS_RX,

  EVENT_BTN_MAXIMUM /**< EVENT_BTN_MAX define the last enum type*/
} APP_BUTTON_EVENT;

typedef enum APP_EVENT_APP_
{
  EVENT_EMPTYY = 127,//DEFINE_EVENT_APP_NBR,(EVENTS ARE MISMATCHING BECAUSE I HAVE MODIFIED ALL THE EVENTS AS PER MY REQUIREMENT
  EVENT_APP_INIT,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_FLUSHMEM_READY,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_SEND_OVERLOAD_NOTIFICATION,
  EVENT_APP_SMARTSTART_IN_PROGRESS,
  EVENT_APP_LEARN_IN_PROGRESS
}
EVENT_APPP;

typedef enum APP_BUTTON_STATE_
{
	DOWN,
	UP,
	SHORT_PRESS,
	HOLD,
	LONG_PRESS
}APP_BUTTON_STATE;

typedef enum APP_BUTTON_TYPE_
{
	NO_TYPE,
	PUSH_BUTTON_TYPE,
	TOGGLE_TYPE,
	SINGLE_STATE_SWITCH_TYPE
} APP_BUTTON_TYPE;

typedef enum APP_BUTTON_ID_
{
	BUTTON_PB,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	TOTAL_NUM_OF_BUTTONS
} APP_BUTTON_ID;


typedef struct _APPLICATION_DATA
{
	uint8_t EP1_OnOffState;
	uint8_t EP2_OnOffState;
	uint8_t EP3_OnOffState;
	uint8_t EP4_OnOffState;
}SApplicationData;


extern SApplicationData  ApplicationData;



void Init_HW();
uint32_t Board_HW_Init(void);



void ZCD_CB(uint8_t pin);
void Sw_Intr_Check();
void Relay_Trigger_Check();

void Set_Sw_Type(APP_BUTTON_TYPE type,APP_BUTTON_ID id);




#endif /* SRC_BOARD_CONFIG_H_ */
