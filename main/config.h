/*
 * Configuracion del sistema: pines, calibracion y tiempos.
 * Ajusta aqui los valores sin tocar el resto del codigo.
 */
#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

/* ---------- Modo de operacion ---------- */
/* 1 = solo imprime lecturas del sensor de suelo para calibrar (no WiFi, no bomba) */
#define MODO_CALIBRACION        0

/* ---------- Pines (ver docs/conexiones.md) ---------- */
#define PIN_RELE                GPIO_NUM_26     /* IN1 del modulo de reles  (E7)  */
#define PIN_DHT22               GPIO_NUM_4      /* DATA del DHT22           (f5)  */
#define SUELO_ADC_CANAL         ADC_CHANNEL_6   /* GPIO34, AO del sensor    (E12) */

/* La mayoria de modulos de rele se activan con nivel BAJO */
#define RELE_ACTIVO_EN_BAJO     1

/* ---------- Calibracion del sensor de suelo ---------- */
/* Usa MODO_CALIBRACION 1 para obtener estos valores con tu sensor */
#define SUELO_VALOR_SECO        4095    /* lectura con la sonda en tierra seca / aire */
#define SUELO_VALOR_MOJADO      850     /* lectura con la sonda en tierra mojada / agua */
#define SUELO_MUESTRAS          16      /* lecturas promediadas por medicion */

/* ---------- Riego ---------- */
#define UMBRAL_INICIAL_PCT      35      /* regar si la humedad baja de este % */
#define UMBRAL_MIN_PCT          5       /* limites aceptados desde el dashboard */
#define UMBRAL_MAX_PCT          95
#define TIEMPO_RIEGO_MS         3000    /* tiempo que se enciende la bomba */
#define ESPERA_ABSORCION_MS     20000   /* espera antes de volver a evaluar */

/* Seguridad: si se riega N veces seguidas y la humedad no sube,
 * se bloquea el riego automatico (posible deposito vacio o sensor fuera de la tierra). */
#define MAX_RIEGOS_SIN_MEJORA   3
#define MEJORA_MINIMA_PCT       3

/* ---------- Tiempos ---------- */
#define PERIODO_LECTURA_MS      2500    /* el DHT22 necesita >= 2 s entre lecturas */
#define PUBLICAR_CADA_MS        20000   /* Adafruit IO gratis: max 30 datos/min */
#define PAUSA_THROTTLE_MS       60000   /* pausa si Adafruit IO nos limita */
#define DHT_FALLOS_PARA_ALERTA  5

/* ---------- Feeds de Adafruit IO (keys) ---------- */
#define FEED_HUMEDAD_SUELO      "humedad-suelo"
#define FEED_TEMPERATURA        "temperatura"
#define FEED_HUMEDAD_AIRE       "humedad-aire"
#define FEED_BOMBA              "bomba"
#define FEED_ALERTAS            "alertas"
#define FEED_ESTADO             "estado"
#define FEED_RIEGO_MANUAL       "riego-manual"   /* suscripcion: boton del dashboard */
#define FEED_UMBRAL             "umbral"         /* suscripcion: slider del dashboard */
