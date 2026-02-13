/*
 * driver_apds9960_interface.c
 *
 *  Created on: 15-Dec-2023
 *      Author: Confio
 */
#include <iic/driver_interface.h>
#include <stdlib.h>
#include "em_i2c.h"
#include "em_cmu.h"
#include "App_Scr/App_Main.h"
/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t interface_iic_init(void)
{
	emberAfCorePrintln("iic Init....\n");
	CMU_ClockEnable(cmuClock_I2C0, true);
	// Use default settings
	I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
	// Use ~400khz SCK
	i2cInit.freq = I2C_FREQ_STANDARD_MAX;
	// Use 6:3 low high SCK ratio
	i2cInit.clhr = i2cClockHLRStandard;

	GPIO_PinModeSet(IIC_SCL_PORT, IIC_SCL_PIN, gpioModeWiredAndPullUpFilter, 1);
	GPIO_PinModeSet(IIC_SDA_PORT, IIC_SDA_PIN, gpioModeWiredAndPullUpFilter, 1);


	// Enable I2C SDA and SCL pins, see the readme or the device datasheet for I2C pin mappings
	// Route I2C pins to GPIO
  GPIO->I2CROUTE[0].SDAROUTE = (GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
                      | (IIC_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                      | (IIC_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SCLROUTE = (GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
                      | (IIC_SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                      | (IIC_SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;


	// Initializing the I2C
	I2C_Init(I2C0, &i2cInit);

	// Setting up to enable slave mode
	I2C0->SADDR = 0x49;
	I2C0->SADDRMASK = 0xFE;
	I2C0->CTRL = I2C_CTRL_AUTOSN;
	//I2C0->CTRL |= I2C_CTRL_SLAVE;



	emberAfCorePrintln("iic Init.... Done\n");
  return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t apds9960_interface_iic_deinit(void)
{
    return 0;
}

void printHex(uint8_t *data, uint16_t len)
{
	//UNUSED(data);
	for(uint16_t i=0; i<len; i++)
	{
		emberAfCorePrintln("%02X ", data[i]);
	}
	emberAfCorePrintln("\n");
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr is iic device write address
 * @param[in]  reg is iic register address
 * @param[out] *buf points to a data buffer
 * @param[in]  len is the length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t apds9960_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
	//emberAfCorePrintlnF("apds9960_read....%02X    %02X\n",addr, reg);
	// Transfer structure
	I2C_TransferSeq_TypeDef i2cTransfer;
	I2C_TransferReturn_TypeDef result;

	// Initializing I2C transfer
	i2cTransfer.addr          = addr;
	i2cTransfer.flags         = I2C_FLAG_WRITE_READ;
	i2cTransfer.buf[0].data   = &reg;
	i2cTransfer.buf[0].len    = 1;
	i2cTransfer.buf[1].data   = buf;
	i2cTransfer.buf[1].len    = len;
	result = I2C_TransferInit(I2C0, &i2cTransfer);

	// Sending data
	while (result == i2cTransferInProgress)
	{
		result = I2C_Transfer(I2C0);
	}
	//emberAfCorePrintlnF("err %d\n", result);
	//printHex(buf, len);
	//emberAfCorePrintln("apds9960_read....   Done\n");

	return 0;

}



/**
 * @brief     interface iic bus write
 * @param[in] addr is iic device write address
 * @param[in] reg is iic register address
 * @param[in] *buf points to a data buffer
 * @param[in] len is the length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t apds9960_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
	//emberAfCorePrintln("writeReg....%02X %02X\n",addr, reg);
	// Transfer structure
	I2C_TransferSeq_TypeDef i2cTransfer;
	I2C_TransferReturn_TypeDef result;

	uint8_t *i2c_txBuffer = (uint8_t*)calloc(len+1, sizeof(uint8_t));
	i2c_txBuffer[0] = reg;
	memcpy(&i2c_txBuffer[1],buf,len);
	uint8_t i2c_rxBuffer[1];

	//printHex(i2c_txBuffer, len+1);

	// Initializing I2C transfer
	i2cTransfer.addr          = addr;
	i2cTransfer.flags         = I2C_FLAG_WRITE;
	i2cTransfer.buf[0].data   = i2c_txBuffer;
	i2cTransfer.buf[0].len    = len+1;
	i2cTransfer.buf[1].data   = i2c_rxBuffer;
	i2cTransfer.buf[1].len    = 0;
	result = I2C_TransferInit(I2C0, &i2cTransfer);

	// Sending data
	while (result == i2cTransferInProgress)
	{
		result = I2C_Transfer(I2C0);
	}
//	emberAfCorePrintln("err %d\n", result);
//	emberAfCorePrintln("writeReg....   Done\n");

	free(i2c_txBuffer);
    return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void apds9960_interface_delay_ms(uint32_t ms)
{
	//UNUSED(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void apds9960_interface_debug_print(const char *const fmt, ...)
{
	//UNUSED(fmt);
	emberAfCorePrintln(fmt);
}

/**
 * @brief     interface receive callback
 * @param[in] type is the interrupt type
 * @note      none
 */
void apds9960_interface_receive_callback(uint8_t type)
{
    switch (type)
    {
        case APDS9960_INTERRUPT_STATUS_GESTURE_LEFT :
        {
            apds9960_interface_debug_print("apds9960: irq gesture left.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_RIGHT :
        {
            apds9960_interface_debug_print("apds9960: irq gesture right.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_UP :
        {
            apds9960_interface_debug_print("apds9960: irq gesture up.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_DOWN :
        {
            apds9960_interface_debug_print("apds9960: irq gesture down.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_NEAR :
        {
            apds9960_interface_debug_print("apds9960: irq gesture near.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GESTURE_FAR :
        {
            apds9960_interface_debug_print("apds9960: irq gesture far.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GFOV :
        {
            apds9960_interface_debug_print("apds9960: irq gesture fifo overflow.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GVALID :
        {
            apds9960_interface_debug_print("apds9960: irq gesture fifo data.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_CPSAT :
        {
            apds9960_interface_debug_print("apds9960: irq clear photo diode saturation.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PGSAT :
        {
            apds9960_interface_debug_print("apds9960: irq analog saturation.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PINT :
        {
            apds9960_interface_debug_print("apds9960: irq proximity interrupt.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_AINT :
        {
            apds9960_interface_debug_print("apds9960: irq als interrupt.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_GINT :
        {
            apds9960_interface_debug_print("apds9960: irq gesture interrupt.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_PVALID :
        {
            apds9960_interface_debug_print("apds9960: irq proximity valid.\n");

            break;
        }
        case APDS9960_INTERRUPT_STATUS_AVALID :
        {
            apds9960_interface_debug_print("apds9960: irq als valid.\n");

            break;
        }
        default :
        {
            apds9960_interface_debug_print("apds9960: irq unknown.\n");

            break;
        }
    }
}
