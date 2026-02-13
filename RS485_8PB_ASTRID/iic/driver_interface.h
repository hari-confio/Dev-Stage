/*
 * driver_interface.h
 *
 *  Created on: 15-Dec-2023
 *      Author: Confio
 */

#ifndef INTERFACE_IIC_H_
#define INTERFACE_IIC_H_



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


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
uint8_t interface_iic_deinit(void);

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
uint8_t interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

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
uint8_t interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);


#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_IIC_H_ */
