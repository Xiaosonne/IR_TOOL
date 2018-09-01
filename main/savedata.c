/*
 * savedata.c
 *
 *  Created on: Jun 8, 2018
 *      Author: markin
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "include/savedata.h"
#include "esp_system.h"
#include "esp_err.h"

#define STORAGE_NAMESPACE "storage"

esp_err_t writeData(char *str, char *data)
{
  char *temp = str;
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Write
  printf("%s \r\n",data);
  err = nvs_set_str(my_handle, temp, data);
  if (err != ESP_OK) return err;

  err = nvs_commit(my_handle);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t readData(char *str, char *data, unsigned int *length)
{
  char *temp = str;
  nvs_handle my_handle;
  esp_err_t err;
  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Read restart counter
  size_t required_size;
  err = nvs_get_str(my_handle, temp, NULL, &length);
  char* dataRead = malloc(length);
  err = nvs_get_str(my_handle, temp, dataRead, &length);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
  memcpy(data,dataRead,length);
  free(dataRead);
  // Close
  nvs_close(my_handle);
  return ESP_OK;
}
