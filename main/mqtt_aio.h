/*
 * Cliente MQTT para Adafruit IO (io.adafruit.com, puerto 8883 con TLS).
 *
 * Topicos:  <usuario>/feeds/<feed_key>
 * Publica telemetria y se suscribe a los feeds de control del dashboard.
 */
#pragma once

#include <stdbool.h>

/* Se llama cuando llega un valor a un feed suscrito. */
typedef void (*aio_callback_t)(const char *feed, const char *valor);

void aio_iniciar(aio_callback_t callback);

bool aio_conectado(void);

/* true si Adafruit IO avisó que excedimos el limite de datos y hay que esperar */
bool aio_limitado(void);

/* Publica un valor en un feed. Devuelve false si no hay conexion o fallo. */
bool aio_publicar(const char *feed, const char *valor);
