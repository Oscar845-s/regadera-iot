/*
 * Pruebas unitarias de la logica de riego. Se ejecutan en la PC, sin ESP32:
 *     cd test && make
 */
#include <stdio.h>
#include "riego_logic.h"

static int fallas = 0;
static int pruebas = 0;

#define CHECK(cond) do { \
    pruebas++; \
    if (!(cond)) { fallas++; printf("  FALLA %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static const riego_config_t CFG = {
    .umbral_pct = 35,
    .max_riegos_sin_mejora = 3,
    .mejora_minima_pct = 3,
};

static void test_porcentaje(void)
{
    printf("porcentaje...\n");
    CHECK(riego_porcentaje(4095, 4095, 1300) == 0);     /* seco */
    CHECK(riego_porcentaje(1300, 4095, 1300) == 100);   /* mojado */
    CHECK(riego_porcentaje(2697, 4095, 1300) == 50);    /* a la mitad */
    CHECK(riego_porcentaje(500, 4095, 1300) == 100);    /* fuera de rango: se limita */
    CHECK(riego_porcentaje(4095, 3500, 1300) == 0);
    CHECK(riego_porcentaje(1000, 1000, 1000) == 0);     /* calibracion invalida */
    CHECK(riego_porcentaje(3000, 1000, 3000) == 100);   /* sensor que sube con humedad */
}

static void test_riega_si_seco(void)
{
    printf("riega si esta seco...\n");
    riego_estado_t e;
    riego_init(&e);
    CHECK(riego_decidir(&CFG, &e, 20) == RIEGO_REGAR);
}

static void test_no_riega_si_humedo(void)
{
    printf("no riega si esta humedo...\n");
    riego_estado_t e;
    riego_init(&e);
    CHECK(riego_decidir(&CFG, &e, 60) == RIEGO_NADA);
    CHECK(riego_decidir(&CFG, &e, 35) == RIEGO_NADA);   /* justo en el umbral */
}

static void test_bloqueo_sin_mejora(void)
{
    printf("se bloquea si la humedad no sube (deposito vacio)...\n");
    riego_estado_t e;
    riego_init(&e);
    CHECK(riego_decidir(&CFG, &e, 20) == RIEGO_REGAR);  /* riego 1 */
    CHECK(riego_decidir(&CFG, &e, 20) == RIEGO_REGAR);  /* sin mejora 1, riego 2 */
    CHECK(riego_decidir(&CFG, &e, 21) == RIEGO_REGAR);  /* sin mejora 2, riego 3 */
    CHECK(riego_decidir(&CFG, &e, 21) == RIEGO_BLOQUEAR); /* sin mejora 3 */
    CHECK(e.bloqueado);
    CHECK(riego_decidir(&CFG, &e, 10) == RIEGO_NADA);   /* bloqueado: no riega */
}

static void test_mejora_reinicia_contador(void)
{
    printf("si la humedad sube, no se bloquea...\n");
    riego_estado_t e;
    riego_init(&e);
    /* Tierra muy seca: cada riego sube la humedad, aunque siga bajo el umbral */
    CHECK(riego_decidir(&CFG, &e, 10) == RIEGO_REGAR);
    CHECK(riego_decidir(&CFG, &e, 18) == RIEGO_REGAR);
    CHECK(riego_decidir(&CFG, &e, 25) == RIEGO_REGAR);
    CHECK(riego_decidir(&CFG, &e, 31) == RIEGO_REGAR);
    CHECK(e.riegos_sin_mejora == 0);
    CHECK(riego_decidir(&CFG, &e, 40) == RIEGO_NADA);   /* ya llego al objetivo */
    CHECK(!e.bloqueado);
}

static void test_desbloqueo(void)
{
    printf("el riego manual quita el bloqueo...\n");
    riego_estado_t e;
    riego_init(&e);
    e.bloqueado = true;
    e.riegos_sin_mejora = 3;
    riego_desbloquear(&e);
    CHECK(!e.bloqueado);
    CHECK(riego_decidir(&CFG, &e, 20) == RIEGO_REGAR);
}

int main(void)
{
    test_porcentaje();
    test_riega_si_seco();
    test_no_riega_si_humedo();
    test_bloqueo_sin_mejora();
    test_mejora_reinicia_contador();
    test_desbloqueo();

    printf("\n%d pruebas, %d fallas\n", pruebas, fallas);
    return fallas == 0 ? 0 : 1;
}
