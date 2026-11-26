/*
 * Control del rele que enciende la bomba.
 */
#pragma once

#include <stdbool.h>

void rele_init(void);
void rele_set(bool encendido);
bool rele_encendido(void);
