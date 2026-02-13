/*
 * app_apds9960.h
 *
 *  Created on: 15-Dec-2023
 *      Author: Confio
 */

#ifndef SRC_APDS9960_APP_APDS9960_H_
#define SRC_APDS9960_APP_APDS9960_H_

void app_apds9960_init();
void app_apds9960_eventHandler();
void apds9960_use_proximity(uint8_t proximity);

#endif /* SRC_APDS9960_APP_APDS9960_H_ */
