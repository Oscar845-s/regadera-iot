/*
 * Logica de decision de riego.
 * C puro, sin dependencias de ESP-IDF, para poder probarla en la PC (ver test/).
 */
#pragma once

#include <stdbool.h>

typedef struct {
    int umbral_pct;             /* regar si humedad < umbral */
    int max_riegos_sin_mejora;  /* riegos seguidos sin efecto antes de bloquear */
    int mejora_minima_pct;      /* cuanto debe subir la humedad tras un riego */
} riego_config_t;

typedef struct {
    int  riegos_sin_mejora;
    int  humedad_antes_riego;   /* -1 si no hay riego pendiente de evaluar */
    bool bloqueado;
} riego_estado_t;

typedef enum {
    RIEGO_NADA = 0,
    RIEGO_REGAR,
    RIEGO_BLOQUEAR,             /* se acaba de bloquear: avisar al usuario */
} riego_accion_t;

/* Convierte la lectura cruda del ADC a porcentaje 0-100 segun la calibracion. */
int riego_porcentaje(int lectura, int valor_seco, int valor_mojado);

void riego_init(riego_estado_t *e);

/* Decide que hacer con la humedad actual. Llamar despues de cada espera de absorcion. */
riego_accion_t riego_decidir(const riego_config_t *c, riego_estado_t *e, int humedad_pct);

/* Quita el bloqueo (p. ej. al regar manualmente tras rellenar el deposito). */
void riego_desbloquear(riego_estado_t *e);
