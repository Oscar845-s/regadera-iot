#include "suelo.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_idf_version.h"
#include "config.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
#define SUELO_ATENUACION ADC_ATTEN_DB_12
#else
#define SUELO_ATENUACION ADC_ATTEN_DB_11
#endif

static adc_oneshot_unit_handle_t s_adc;

esp_err_t suelo_init(void)
{
    adc_oneshot_unit_init_cfg_t unidad = { .unit_id = ADC_UNIT_1 };
    esp_err_t err = adc_oneshot_new_unit(&unidad, &s_adc);
    if (err != ESP_OK) {
        return err;
    }

    adc_oneshot_chan_cfg_t canal = {
        .atten = SUELO_ATENUACION,      /* rango 0 - 3.3 V */
        .bitwidth = ADC_BITWIDTH_12,    /* 0 - 4095 */
    };
    return adc_oneshot_config_channel(s_adc, SUELO_ADC_CANAL, &canal);
}

esp_err_t suelo_leer_crudo(int *lectura)
{
    long suma = 0;
    for (int i = 0; i < SUELO_MUESTRAS; i++) {
        int valor;
        esp_err_t err = adc_oneshot_read(s_adc, SUELO_ADC_CANAL, &valor);
        if (err != ESP_OK) {
            return err;
        }
        suma += valor;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    *lectura = (int)(suma / SUELO_MUESTRAS);
    return ESP_OK;
}
