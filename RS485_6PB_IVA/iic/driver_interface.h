/*
 * driver_apds9960_interface.h
 *
 *  Created on: 15-Dec-2023
 *      Author: Confio
 */

#ifndef SRC_APDS9960_DRIVER_APDS9960_INTERFACE_H_
#define SRC_APDS9960_DRIVER_APDS9960_INTERFACE_H_


#include "../apds9960/driver_apds9960.h"



//PC6 (SCL)
#define IIC_SCL_PORT		gpioPortC
#define IIC_SCL_PIN			4

//PC7 (SDA)
#define IIC_SDA_PORT		gpioPortC
#define IIC_SDA_PIN			3

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @defgroup apds9960_interface_driver apds9960 interface driver function
 * @brief    apds9960 interface driver modules
 * @ingroup  apds9960_driver
 * @{
 */

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t interface_iic_init(void);

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t apds9960_interface_iic_deinit(void);

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
uint8_t apds9960_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

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
uint8_t apds9960_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void apds9960_interface_delay_ms(uint32_t ms);

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void apds9960_interface_debug_print(const char *const fmt, ...);

/**
 * @brief     interface receive callback
 * @param[in] type is the interrupt type
 * @note      none
 */
void apds9960_interface_receive_callback(uint8_t type);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SRC_APDS9960_DRIVER_APDS9960_INTERFACE_H_ */
