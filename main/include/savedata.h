/*
 * savedata.h
 *
 *  Created on: Jun 8, 2018
 *      Author: markin
 */

#ifndef SAVEDATA_H_
#define SAVEDATA_H_

#include <stdint.h>
#include "nvs_flash.h"
#include "nvs.h"

esp_err_t writeData(char *str, char* data);
esp_err_t readData(char *str, char *data, unsigned int *length);

#endif /* SAVEDATA_H_ */
