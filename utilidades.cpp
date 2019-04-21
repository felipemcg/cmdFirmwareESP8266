/*
 * utilidades.cpp
 *
 *  Created on: 16 abr. 2019
 *      Author: felip
 */
#include "Arduino.h"

/* Descripcion: Verifica el numero val este dentro del rango limitado por min y max*/
bool dentro_intervalo(uint32_t val, uint32_t min, uint32_t max)
{
  return ((min <= val) && (val <= max));
}



