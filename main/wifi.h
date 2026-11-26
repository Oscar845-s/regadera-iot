/*
 * Conexion WiFi en modo estacion con reconexion automatica.
 */
#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

void wifi_iniciar(void);

/* Bloquea hasta tener IP o hasta que pase 'espera'. Devuelve true si hay conexion. */
bool wifi_esperar_conexion(TickType_t espera);

bool wifi_conectado(void);
