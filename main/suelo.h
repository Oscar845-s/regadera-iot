/*
 * Sensor de humedad de suelo (salida analogica AO) en ADC1.
 */
#pragma once

#include "esp_err.h"

esp_err_t suelo_init(void);

/* Lectura cruda promediada (0-4095). */
esp_err_t suelo_leer_crudo(int *lectura);
