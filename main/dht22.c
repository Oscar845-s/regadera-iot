#include "dht22.h"

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

/* Espera mientras la linea este en 'nivel'. Devuelve los us que duro, o -1 si excede el limite. */
static int esperar_mientras(gpio_num_t pin, int nivel, int limite_us)
{
    int64_t inicio = esp_timer_get_time();
    while (gpio_get_level(pin) == nivel) {
        if (esp_timer_get_time() - inicio > limite_us) {
            return -1;
        }
    }
    return (int)(esp_timer_get_time() - inicio);
}

void dht22_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_level(pin, 1);
}

esp_err_t dht22_leer(gpio_num_t pin, float *temperatura, float *humedad)
{
    uint8_t datos[5] = {0};
    esp_err_t res = ESP_OK;

    /* Senal de inicio: linea en bajo >= 1 ms */
    gpio_set_level(pin, 0);
    esp_rom_delay_us(1500);

    portENTER_CRITICAL(&s_mux);
    gpio_set_level(pin, 1);

    /* Respuesta del sensor: ~20-40 us alto, 80 us bajo, 80 us alto */
    if (esperar_mientras(pin, 1, 80) < 0 ||
        esperar_mientras(pin, 0, 100) < 0 ||
        esperar_mientras(pin, 1, 100) < 0) {
        res = ESP_ERR_TIMEOUT;
    } else {
        /* 40 bits: 50 us bajo + alto de ~27 us (0) o ~70 us (1) */
        for (int i = 0; i < 40; i++) {
            if (esperar_mientras(pin, 0, 80) < 0) {
                res = ESP_ERR_TIMEOUT;
                break;
            }
            int alto = esperar_mientras(pin, 1, 100);
            if (alto < 0) {
                res = ESP_ERR_TIMEOUT;
                break;
            }
            datos[i / 8] <<= 1;
            if (alto > 40) {
                datos[i / 8] |= 1;
            }
        }
    }
    portEXIT_CRITICAL(&s_mux);

    if (res != ESP_OK) {
        return res;
    }

    uint8_t suma = (uint8_t)(datos[0] + datos[1] + datos[2] + datos[3]);
    if (suma != datos[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    *humedad = ((datos[0] << 8) | datos[1]) / 10.0f;
    float t = (((datos[2] & 0x7F) << 8) | datos[3]) / 10.0f;
    *temperatura = (datos[2] & 0x80) ? -t : t;
    return ESP_OK;
}
