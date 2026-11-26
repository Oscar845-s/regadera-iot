#include "riego_logic.h"

int riego_porcentaje(int lectura, int valor_seco, int valor_mojado)
{
    if (valor_seco == valor_mojado) {
        return 0;
    }
    /* Funciona tanto si el sensor baja con la humedad como si sube */
    int pct = (valor_seco - lectura) * 100 / (valor_seco - valor_mojado);
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void riego_init(riego_estado_t *e)
{
    e->riegos_sin_mejora = 0;
    e->humedad_antes_riego = -1;
    e->bloqueado = false;
}

riego_accion_t riego_decidir(const riego_config_t *c, riego_estado_t *e, int humedad_pct)
{
    if (e->bloqueado) {
        return RIEGO_NADA;
    }

    /* Evaluar si el riego anterior tuvo efecto */
    if (e->humedad_antes_riego >= 0) {
        if (humedad_pct - e->humedad_antes_riego >= c->mejora_minima_pct) {
            e->riegos_sin_mejora = 0;
        } else {
            e->riegos_sin_mejora++;
        }
        e->humedad_antes_riego = -1;
    }

    if (e->riegos_sin_mejora >= c->max_riegos_sin_mejora) {
        e->bloqueado = true;
        return RIEGO_BLOQUEAR;
    }

    if (humedad_pct < c->umbral_pct) {
        e->humedad_antes_riego = humedad_pct;
        return RIEGO_REGAR;
    }

    e->riegos_sin_mejora = 0;
    return RIEGO_NADA;
}

void riego_desbloquear(riego_estado_t *e)
{
    e->bloqueado = false;
    e->riegos_sin_mejora = 0;
    e->humedad_antes_riego = -1;
}
