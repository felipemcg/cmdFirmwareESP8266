/*
 * operaciones_paquetes.h
 *
 *  Created on: 19 nov. 2018
 *      Author: felip
 */

#ifndef OPERACIONES_PAQUETES_H_
#define OPERACIONES_PAQUETES_H_

bool recibir_paquetes(char *paquete_serial, char *paquete_datos_recibidos_tcp);
struct cmd_recibido separar_comando_parametros(char *paquete);

#endif /* OPERACIONES_PAQUETES_H_ */
