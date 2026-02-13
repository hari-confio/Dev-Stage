
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <board_config.h>
#include "DebugPrintConfig.h"
#if defined(APP_DEBUG)
	#define DEBUGPRINT
#endif
#include "DebugPrint.h"

#include <stdint.h>

#include "stdbool.h"

#include <ZW_application_transport_interface.h>

#include "zaf_event_distributor_soc.h"

#include <AppTimer.h>

#include "gpiointerrupt.h"

#include "ZAF_uart_utils.h"
#include "em_cmu.h"

#include "em_timer.h"
#include "em_emu.h"
#include "SizeOf.h"
#include "App_Serial.h"
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

//Disabling USART0 saves much power when an application is battery powered
//#define DISABLE_USART0

uint8_t  Button[TOTAL_NUM_OF_BUTTONS];


#ifdef APP_DEBUG
static uint8_t m_aDebugPrintBuffer[96];
#endif
/**
 * @brief GPIO EM4 wakeup flags (bitmask containing button(s) that woke us up from EM4)
 */
uint32_t g_gpioEm4Flags = 0;


// Learn Reset button Interrupt CB
void Interrupt_CB(uint8_t pin)
{
  UNUSED(pin);
	if(false == GPIO_PinInGet(LEARN_RESET_BTN_PORT, LEARN_RESET_BTN_PIN))
	{
	    zaf_event_distributor_enqueue_app_event_from_isr(APP_EVENT_PB_DOWN);
	}
	else
	{
	    zaf_event_distributor_enqueue_app_event(APP_EVENT_SW1_TRIPLE_PRESS);//include/exclude event
	}
}



//Board HW init
void Init_HW()
{
	CMU_ClockEnable(cmuClock_GPIO, true);

	//Keeping all GPIO to output mode and logic LOW
	//Port A
	//GPIO_PinModeSet(PORT_A0, PIN_A0, OutputMode, 0);
	//GPIO_PinModeSet(PORT_A1, PIN_A1, OutputMode, 0);
	GPIO_PinModeSet(PORT_A2, PIN_A2, OutputMode, 0);
	GPIO_PinModeSet(PORT_A3, PIN_A3, OutputMode, 0);
	GPIO_PinModeSet(PORT_A4, PIN_A4, OutputMode, 0);
	GPIO_PinModeSet(PORT_A5, PIN_A5, OutputMode, 0);

	//Port B
	GPIO_PinModeSet(PORT_B11, PIN_B11, OutputMode, 0);
	GPIO_PinModeSet(PORT_B12, PIN_B12, OutputMode, 0);
	GPIO_PinModeSet(PORT_B13, PIN_B13, OutputMode, 0);
	GPIO_PinModeSet(PORT_B14, PIN_B14, OutputMode, 0);
	GPIO_PinModeSet(PORT_B15, PIN_B15, OutputMode, 0);


	//Port C
	GPIO_PinModeSet(PORT_C6, PIN_C6, OutputMode, 0);
	GPIO_PinModeSet(PORT_C7, PIN_C7, OutputMode, 0);
	GPIO_PinModeSet(PORT_C8, PIN_C8, OutputMode, 0);
	GPIO_PinModeSet(PORT_C9, PIN_C9, OutputMode, 0);
	GPIO_PinModeSet(PORT_C10, PIN_C10, OutputMode, 0);
	GPIO_PinModeSet(PORT_C11, PIN_C11, OutputMode, 0);

	//Port D
	GPIO_PinModeSet(PORT_D9, PIN_D9, OutputMode, 0);
	GPIO_PinModeSet(PORT_D10, PIN_D10, OutputMode, 0);
	GPIO_PinModeSet(PORT_D11, PIN_D11, OutputMode, 0);
	GPIO_PinModeSet(PORT_D12, PIN_D12, OutputMode, 0);
	//GPIO_PinModeSet(PORT_D13, PIN_D13, OutputMode, 0);
	GPIO_PinModeSet(PORT_D14, PIN_D14, OutputMode, 0);
	GPIO_PinModeSet(PORT_D15, PIN_D15, OutputMode, 0);


	//Port F
	GPIO_PinModeSet(PORT_F0, PIN_F0, OutputMode, 0);
	GPIO_PinModeSet(PORT_F1, PIN_F1, OutputMode, 0);
	GPIO_PinModeSet(PORT_F2, PIN_F2, OutputMode, 0);
	GPIO_PinModeSet(PORT_F3, PIN_F3, OutputMode, 0);
	GPIO_PinModeSet(PORT_F4, PIN_F4, OutputMode, 0);
	GPIO_PinModeSet(PORT_F5, PIN_F5, OutputMode, 0);
	GPIO_PinModeSet(PORT_F6, PIN_F6, OutputMode, 0);
	GPIO_PinModeSet(PORT_F7, PIN_F7, OutputMode, 0);

#if defined(APP_DEBUG)
	ZAF_UART0_init(UART0_TX_PORT, UART0_TX_PIN, UART0_RX_PORT, UART0_RX_PIN);
	ZAF_UART0_enable(115200, true, false);
	DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), ZAF_UART0_tx_send);
#else
	GPIO_PinModeSet(UART0_TX_PORT, UART0_TX_PIN, OutputMode, 1);
	GPIO_PinModeSet(UART0_RX_PORT, UART0_RX_PIN, OutputMode, 1);
#endif
}

uint32_t Board_HW_Init(void)									//GPIO INIT
{
	CMU_ClockEnable(cmuClock_GPIO, true);

	/* Unlatch EM4 GPIO pin states after wakeup (OK to call even if not EM4 wakeup) */
	EMU_UnlatchPinRetention();

	/* Save the EM4 GPIO wakeup flags */
	g_gpioEm4Flags = GPIO_IntGet() & _GPIO_IF_EM4WU_MASK;
	GPIO_IntClear(g_gpioEm4Flags);

	GPIOINT_Init();
	NVIC_SetPriority(GPIO_ODD_IRQn, 5);
	NVIC_SetPriority(GPIO_EVEN_IRQn, 5);

	GPIO_PinModeSet(LEARN_RESET_BTN_PORT, LEARN_RESET_BTN_PIN, InputMode, 1);			//Exclude/Include button

	GPIO_ExtIntConfig(LEARN_RESET_BTN_PORT, LEARN_RESET_BTN_PIN, LEARN_RESET_BTN_PIN, true, true, true);
	GPIOINT_CallbackRegister(LEARN_RESET_BTN_PIN, Interrupt_CB);

//	Serail_Init();
//  Serail_Init1();

	return 0;
}


