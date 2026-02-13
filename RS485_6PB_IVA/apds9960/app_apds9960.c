/*
 * app_apds9960.c
 *
 *  Created on: 15-Dec-2023
 *      Author: Confio
 */

#include "driver_apds9960.h"
#include "app_apds9960_interrupt.h"
#include <iic/driver_interface.h>
#include "gpiointerrupt.h"
#include "app_apds9960.h"
#include "App_Scr/App_Main.h"

//EmberEventControl sensorEventControl;

static void a_interrupt_callback(uint8_t type)
{
    switch (type)
    {
        case APDS9960_INTERRUPT_STATUS_GESTURE_LEFT :
        {
        	emberAfCorePrintln("apds9960: irq gesture left.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_RIGHT :
        {
        	emberAfCorePrintln("apds9960: irq gesture right.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_UP :
        {
        	emberAfCorePrintln("apds9960: irq gesture up.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_DOWN :
        {
        	emberAfCorePrintln("apds9960: irq gesture down.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_NEAR :
        {
        	emberAfCorePrintln("apds9960: irq gesture near.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_FAR :
        {
        	emberAfCorePrintln("apds9960: irq gesture far.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GFOV :
        {
        	emberAfCorePrintln("apds9960: irq gesture fifo overflow.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GVALID :
        {
        	uint8_t res = apds9960_interrupt_read_gestureBuff();
        	//UNUSED(res);
        	emberAfCorePrintln("apds9960: irq gesture fifo data.  %d \n", res);

            break;
        }
        case APDS9960_INTERRUPT_STATUS_CPSAT :
        {
        	emberAfCorePrintln("apds9960: irq clear photodiode saturation.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PGSAT :
        {
        	emberAfCorePrintln("apds9960: irq analog saturation.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PINT :
        {
            uint8_t res;
            uint8_t proximity;

            /* read proximity */
            res = apds9960_interrupt_read_proximity((uint8_t *)&proximity);
            if (res != 0)
            {
            	emberAfCorePrintln("apds9960: read proximity failed.\n");
            }
            emberAfCorePrintln("apds9960: proximity is 0x%02X.\n", proximity);
            apds9960_use_proximity(proximity);
            break;
        }
        case APDS9960_INTERRUPT_STATUS_AINT :
        {
            uint8_t res;
            uint16_t red, green, blue, clear;

            /* read rgbc */
            res = apds9960_interrupt_read_rgbc((uint16_t *)&red, (uint16_t *)&green, (uint16_t *)&blue, (uint16_t *)&clear);
            if (res != 0)
            {
            	emberAfCorePrintln("apds9960: read rgbc failed.\n");
                return;
            }
            /* output */
            emberAfCorePrintln("apds9960: red is 0x%04X.\n", red);
            emberAfCorePrintln("apds9960: green is 0x%04X.\n", green);
            emberAfCorePrintln("apds9960: blue is 0x%04X.\n", blue);
            emberAfCorePrintln("apds9960: clear is 0x%04X.\n", clear);

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GINT :
        {
        	emberAfCorePrintln("apds9960: irq gesture interrupt.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PVALID :
        {
            break;
        }
        case APDS9960_INTERRUPT_STATUS_AVALID :
        {
            break;
        }
        default :
        {
        	emberAfCorePrintln("apds9960: irq unknown.\n");

            break;
        }
    }
}

//void Sensor_interrupt_CB(uint8_t id)
//{
  //emberAfCorePrintln("Sensor_interrupt_CB");
//  emberEventControlSetActive(sensorEventControl);
//}

//void sensorEventHandler(void)
//{
//  emberEventControlSetInactive(sensorEventControl);
//  app_apds9960_eventHandler();
//}


void app_apds9960_eventHandler()
{
	apds9960_interrupt_irq_handler();
}



void app_apds9960_init()
{
	//GPIO_PinModeSet(PORT_SENSOR_INT, PIN_SENSOR_INT, gpioModeInputPull, 1);
	//GPIO_ExtIntConfig(PORT_SENSOR_INT, PIN_SENSOR_INT, PIN_SENSOR_INT, true, true, true);
	//GPIOINT_CallbackRegister(PIN_SENSOR_INT, Sensor_interrupt_CB);
	/* run interrupt function */
	if (apds9960_interrupt_init(a_interrupt_callback,
								1,
	              6000,
								1,
								30) != 0)
	{
	    return;
	}
}

