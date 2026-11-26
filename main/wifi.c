#include "wifi.h"

#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "secrets.h"

static const char *TAG = "wifi";
static EventGroupHandle_t s_eventos;
#define BIT_CONECTADO BIT0

static void manejador(void *arg, esp_event_base_t base, int32_t id, void *datos)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)datos;
        xEventGroupClearBits(s_eventos, BIT_CONECTADO);
        ESP_LOGW(TAG, "Desconectado (motivo %d). Reintentando...", d->reason);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)datos;
        ESP_LOGI(TAG, "Conectado, IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_eventos, BIT_CONECTADO);
    }
}

void wifi_iniciar(void)
{
    s_eventos = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &manejador, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &manejador, NULL, NULL));

    wifi_config_t wcfg = { 0 };
    strncpy((char *)wcfg.sta.ssid, WIFI_SSID, sizeof(wcfg.sta.ssid) - 1);
    strncpy((char *)wcfg.sta.password, WIFI_PASS, sizeof(wcfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Conectando a \"%s\"...", WIFI_SSID);
}

bool wifi_esperar_conexion(TickType_t espera)
{
    EventBits_t bits = xEventGroupWaitBits(s_eventos, BIT_CONECTADO, pdFALSE, pdTRUE, espera);
    return (bits & BIT_CONECTADO) != 0;
}

bool wifi_conectado(void)
{
    return (xEventGroupGetBits(s_eventos) & BIT_CONECTADO) != 0;
}
