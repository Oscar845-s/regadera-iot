#include "rele.h"

#include "driver/gpio.h"
#include "config.h"

static bool s_encendido = false;

static int nivel_para(bool encendido)
{
#if RELE_ACTIVO_EN_BAJO
    return encendido ? 0 : 1;
#else
    return encendido ? 1 : 0;
#endif
}

void rele_init(void)
{
    /* Fijar el nivel de "apagado" antes de habilitar la salida evita un pulso al arrancar */
    gpio_reset_pin(PIN_RELE);
    gpio_set_level(PIN_RELE, nivel_para(false));
    gpio_set_direction(PIN_RELE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELE, nivel_para(false));
    s_encendido = false;
}

void rele_set(bool encendido)
{
    gpio_set_level(PIN_RELE, nivel_para(encendido));
    s_encendido = encendido;
}

bool rele_encendido(void)
{
    return s_encendido;
}
