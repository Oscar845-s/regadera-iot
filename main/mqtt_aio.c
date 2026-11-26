#include "mqtt_aio.h"

#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"
#include "secrets.h"

static const char *TAG = "aio";

static esp_mqtt_client_handle_t s_cliente;
static aio_callback_t s_callback;
static volatile bool s_conectado = false;
static volatile int64_t s_limitado_hasta_us = 0;

static char s_topico_estado[96];

static void topico_feed(char *buf, size_t n, const char *feed)
{
    snprintf(buf, n, "%s/feeds/%s", AIO_USERNAME, feed);
}

static void suscribir(const char *feed, bool pedir_ultimo)
{
    char topico[96];
    topico_feed(topico, sizeof(topico), feed);
    esp_mqtt_client_subscribe(s_cliente, topico, 1);

    if (!pedir_ultimo) {
        return;
    }
    /* Pedir el ultimo valor guardado para sincronizar al arrancar */
    char get[104];
    snprintf(get, sizeof(get), "%s/get", topico);
    esp_mqtt_client_publish(s_cliente, get, "", 0, 0, 0);
}

static void al_recibir(esp_mqtt_event_handle_t ev)
{
    char topico[128];
    char valor[64];
    int lt = ev->topic_len < (int)sizeof(topico) - 1 ? ev->topic_len : (int)sizeof(topico) - 1;
    int lv = ev->data_len < (int)sizeof(valor) - 1 ? ev->data_len : (int)sizeof(valor) - 1;
    memcpy(topico, ev->topic, lt);
    topico[lt] = '\0';
    memcpy(valor, ev->data, lv);
    valor[lv] = '\0';

    /* Mensajes de error del propio Adafruit IO */
    char t_throttle[64], t_errors[64];
    snprintf(t_throttle, sizeof(t_throttle), "%s/throttle", AIO_USERNAME);
    snprintf(t_errors, sizeof(t_errors), "%s/errors", AIO_USERNAME);
    if (strcmp(topico, t_throttle) == 0) {
        ESP_LOGW(TAG, "Limite de datos excedido: %s", valor);
        s_limitado_hasta_us = esp_timer_get_time() + (int64_t)PAUSA_THROTTLE_MS * 1000;
        return;
    }
    if (strcmp(topico, t_errors) == 0) {
        ESP_LOGE(TAG, "Error de Adafruit IO: %s", valor);
        return;
    }

    const char *p = strstr(topico, "/feeds/");
    if (p != NULL && s_callback != NULL) {
        s_callback(p + strlen("/feeds/"), valor);
    }
}

static void manejador(void *arg, esp_event_base_t base, int32_t id, void *datos)
{
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)datos;

    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "Conectado a Adafruit IO");
        s_conectado = true;

        char t[64];
        snprintf(t, sizeof(t), "%s/throttle", AIO_USERNAME);
        esp_mqtt_client_subscribe(s_cliente, t, 0);
        snprintf(t, sizeof(t), "%s/errors", AIO_USERNAME);
        esp_mqtt_client_subscribe(s_cliente, t, 0);

        /* riego-manual NO pide su ultimo valor: evitaria regar en cada reconexion */
        suscribir(FEED_RIEGO_MANUAL, false);
        suscribir(FEED_UMBRAL, true);
        esp_mqtt_client_publish(s_cliente, s_topico_estado, "online", 0, 1, 0);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado de Adafruit IO (se reintentara)");
        s_conectado = false;
        break;
    case MQTT_EVENT_DATA:
        al_recibir(ev);
        break;
    case MQTT_EVENT_ERROR:
        if (ev->error_handle != NULL &&
            ev->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGE(TAG, "Conexion rechazada: revisa AIO_USERNAME y AIO_KEY en secrets.h");
        } else {
            ESP_LOGE(TAG, "Error MQTT");
        }
        break;
    default:
        break;
    }
}

void aio_iniciar(aio_callback_t callback)
{
    s_callback = callback;
    topico_feed(s_topico_estado, sizeof(s_topico_estado), FEED_ESTADO);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtts://io.adafruit.com:8883",
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .credentials.username = AIO_USERNAME,
        .credentials.authentication.password = AIO_KEY,
        .session.keepalive = 60,
        .session.last_will = {
            .topic = s_topico_estado,
            .msg = "offline",
            .qos = 1,
            .retain = 0,
        },
        .network.reconnect_timeout_ms = 5000,
    };

    s_cliente = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_cliente, ESP_EVENT_ANY_ID, manejador, NULL);
    esp_mqtt_client_start(s_cliente);
}

bool aio_conectado(void)
{
    return s_conectado;
}

bool aio_limitado(void)
{
    return esp_timer_get_time() < s_limitado_hasta_us;
}

bool aio_publicar(const char *feed, const char *valor)
{
    if (!s_conectado || aio_limitado()) {
        return false;
    }
    char topico[96];
    topico_feed(topico, sizeof(topico), feed);
    int id = esp_mqtt_client_publish(s_cliente, topico, valor, 0, 1, 0);
    if (id < 0) {
        ESP_LOGW(TAG, "No se pudo publicar en %s", feed);
        return false;
    }
    return true;
}
