/*
 * Regadera automatica IoT
 * ESP32 + ESP-IDF + Adafruit IO (MQTT sobre TLS)
 *
 * - Lee humedad de suelo (ADC) y temperatura/humedad del aire (DHT22).
 * - Riega automaticamente cuando la humedad del suelo baja del umbral.
 * - Publica telemetria en Adafruit IO y recibe comandos del dashboard
 *   (riego manual y cambio de umbral).
 * - Sigue regando aunque no haya WiFi (el control es local).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "config.h"
#include "wifi.h"
#include "mqtt_aio.h"
#include "dht22.h"
#include "suelo.h"
#include "rele.h"
#include "riego_logic.h"

static const char *TAG = "regadera";

typedef enum {
    CMD_RIEGO_MANUAL,
} comando_t;

static QueueHandle_t s_comandos;
static volatile int s_umbral_pct = UMBRAL_INICIAL_PCT;

static int64_t ahora_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void publicar_int(const char *feed, int valor)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", valor);
    aio_publicar(feed, buf);
}

static void publicar_float(const char *feed, float valor)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", valor);
    aio_publicar(feed, buf);
}

static void alerta(const char *mensaje)
{
    ESP_LOGW(TAG, "ALERTA: %s", mensaje);
    aio_publicar(FEED_ALERTAS, mensaje);
}

/* ---------- Comandos recibidos desde Adafruit IO (tarea del cliente MQTT) ---------- */
static void al_recibir_comando(const char *feed, const char *valor)
{
    ESP_LOGI(TAG, "Recibido %s = %s", feed, valor);

    if (strcmp(feed, FEED_RIEGO_MANUAL) == 0) {
        if (atoi(valor) == 1) {
            comando_t c = CMD_RIEGO_MANUAL;
            if (xQueueSend(s_comandos, &c, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Cola de comandos llena, se ignora el riego manual");
            }
        }
    } else if (strcmp(feed, FEED_UMBRAL) == 0) {
        int u = atoi(valor);
        if (u >= UMBRAL_MIN_PCT && u <= UMBRAL_MAX_PCT) {
            s_umbral_pct = u;
            ESP_LOGI(TAG, "Nuevo umbral: %d %%", u);
        } else {
            ESP_LOGW(TAG, "Umbral fuera de rango (%s), se ignora", valor);
        }
    }
}

/* ---------- Bomba ---------- */
static void regar(const char *motivo)
{
    ESP_LOGI(TAG, ">> Regando %d ms (%s)", TIEMPO_RIEGO_MS, motivo);
    rele_set(true);
    aio_publicar(FEED_BOMBA, "1");
    vTaskDelay(pdMS_TO_TICKS(TIEMPO_RIEGO_MS));
    rele_set(false);
    aio_publicar(FEED_BOMBA, "0");
}

/* ---------- Tarea principal de control ---------- */
static void tarea_control(void *arg)
{
    riego_estado_t estado;
    riego_init(&estado);

    int humedad_suelo = -1;
    float temperatura = 0, humedad_aire = 0;
    bool dht_valido = false;
    int dht_fallos = 0;
    bool alerta_dht_enviada = false;

    int64_t proxima_evaluacion = 0;
    int64_t proxima_publicacion = 0;
    bool estaba_conectado = false;

    while (1) {
        /* 1. Sensor de suelo */
        int crudo = -1;
        if (suelo_leer_crudo(&crudo) == ESP_OK) {
            humedad_suelo = riego_porcentaje(crudo, SUELO_VALOR_SECO, SUELO_VALOR_MOJADO);
        } else {
            ESP_LOGE(TAG, "Error leyendo el sensor de suelo");
        }

        /* 2. DHT22 (con tolerancia a fallos: se conserva el ultimo valor valido) */
        float t, h;
        esp_err_t err = dht22_leer(PIN_DHT22, &t, &h);
        if (err == ESP_OK) {
            temperatura = t;
            humedad_aire = h;
            dht_valido = true;
            dht_fallos = 0;
            alerta_dht_enviada = false;
        } else {
            dht_fallos++;
            ESP_LOGW(TAG, "DHT22 fallo (%s), intento %d", esp_err_to_name(err), dht_fallos);
            if (dht_fallos >= DHT_FALLOS_PARA_ALERTA && !alerta_dht_enviada) {
                alerta("DHT22 sin respuesta: revisar conexion");
                alerta_dht_enviada = true;
                dht_valido = false;
            }
        }

        ESP_LOGI(TAG, "Suelo: %d %% (crudo %d) | Aire: %.1f C, %.1f %% | Umbral: %d %% | %s",
                 humedad_suelo, crudo, temperatura, humedad_aire, s_umbral_pct,
                 aio_conectado() ? "nube OK" : "sin nube");

        /* 3. Comandos del dashboard */
        comando_t cmd;
        if (xQueueReceive(s_comandos, &cmd, 0) == pdTRUE && cmd == CMD_RIEGO_MANUAL) {
            if (estado.bloqueado) {
                ESP_LOGI(TAG, "Riego manual: se quita el bloqueo de seguridad");
            }
            riego_desbloquear(&estado);
            regar("manual desde el dashboard");
            proxima_evaluacion = ahora_ms() + ESPERA_ABSORCION_MS;
        }

        /* 4. Riego automatico */
        if (humedad_suelo >= 0 && ahora_ms() >= proxima_evaluacion) {
            riego_config_t cfg = {
                .umbral_pct = s_umbral_pct,
                .max_riegos_sin_mejora = MAX_RIEGOS_SIN_MEJORA,
                .mejora_minima_pct = MEJORA_MINIMA_PCT,
            };
            switch (riego_decidir(&cfg, &estado, humedad_suelo)) {
            case RIEGO_REGAR:
                regar("humedad baja");
                proxima_evaluacion = ahora_ms() + ESPERA_ABSORCION_MS;
                break;
            case RIEGO_BLOQUEAR:
                alerta("Riego bloqueado: la humedad no sube. Revisar deposito o sensor");
                break;
            default:
                break;
            }
        }

        /* 5. Telemetria a la nube */
        bool conectado = aio_conectado();
        if (conectado && !estaba_conectado) {
            proxima_publicacion = 0;    /* publicar en cuanto se reconecta */
        }
        estaba_conectado = conectado;

        if (conectado && ahora_ms() >= proxima_publicacion) {
            if (humedad_suelo >= 0) {
                publicar_int(FEED_HUMEDAD_SUELO, humedad_suelo);
            }
            if (dht_valido) {
                publicar_float(FEED_TEMPERATURA, temperatura);
                publicar_float(FEED_HUMEDAD_AIRE, humedad_aire);
            }
            proxima_publicacion = ahora_ms() + PUBLICAR_CADA_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(PERIODO_LECTURA_MS));
    }
}

/* ---------- Modo calibracion ---------- */
static void modo_calibracion(void)
{
    int maximo = 0, minimo = 4095;
    ESP_LOGI(TAG, "MODO CALIBRACION: sonda al aire/tierra seca y luego en agua/tierra mojada");
    while (1) {
        int crudo;
        if (suelo_leer_crudo(&crudo) == ESP_OK) {
            if (crudo > maximo) maximo = crudo;
            if (crudo < minimo) minimo = crudo;
            printf("Actual: %4d | SECO (max): %4d | MOJADO (min): %4d\n", crudo, maximo, minimo);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    /* La bomba queda apagada desde el primer momento */
    rele_init();
    ESP_ERROR_CHECK(suelo_init());

    if (MODO_CALIBRACION) {
        modo_calibracion();
    }

    dht22_init(PIN_DHT22);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    s_comandos = xQueueCreate(4, sizeof(comando_t));

    /* El control arranca de inmediato, con o sin WiFi */
    xTaskCreate(tarea_control, "control", 4096, NULL, 5, NULL);

    wifi_iniciar();
    if (!wifi_esperar_conexion(pdMS_TO_TICKS(15000))) {
        ESP_LOGW(TAG, "Sin WiFi por ahora: el riego sigue funcionando en modo local");
        wifi_esperar_conexion(portMAX_DELAY);
    }
    aio_iniciar(al_recibir_comando);
}
