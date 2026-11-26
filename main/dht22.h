/*
 * Driver minimo del DHT22 / AM2302 (protocolo de un solo hilo), sin librerias externas.
 */
#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

void dht22_init(gpio_num_t pin);

/*
 * Lee temperatura (C) y humedad relativa (%).
 * Devuelve ESP_OK, ESP_ERR_TIMEOUT (sensor no responde) o ESP_ERR_INVALID_CRC (dato corrupto).
 * No llamar mas de una vez cada 2 segundos.
 */
esp_err_t dht22_leer(gpio_num_t pin, float *temperatura, float *humedad);
