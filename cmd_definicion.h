/*
 * cmd_definicion.h
 *
 *  Created on: 19 nov. 2018
 *      Author: felip
 */

#ifndef CMD_DEFINICION_H_
#define CMD_DEFINICION_H_
#include "Arduino.h"

#define CMD_TERMINATOR '\n'
#define CMD_DELIMITER ','
#define CMD_RESP_OK '0'
#define CMD_NOT_FOUND ''
#define CMD_NO_PARAM ''

/*Cantidad maxima de parametros por comando*/
#define CANT_MAX_PARAMETROS_CMD 5

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define CANT_MAX_CARACT_PARAMETRO 64

/*Cantidad maxima de caracteres que puede contener el nombre del comando*/
#define CANT_MAX_CARACT_NOMBRE_CMD 3

/*Numero maximo de comandos admitidos*/
#define CANT_MAX_CMD 25

/*Tamaño del buffer serial*/
#define TAM_BUFFER_SERIAL 1024

/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define TIEMPO_MS_ESPERA_RESPUESTA_SERVIDOR 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define TIEMPO_MS_ESPERA_CONEXION_WIFI 20000

/*Maximum number of Bytes for a packet*/
#define TAM_MAX_PAQUETE_DATOS_TCP 1460

/*Numero maximo de clientes que puede manejar el modulo*/
#define CANT_MAX_CLIENTES 4

/*Numero maximo de servidores que puede manejar el modulo*/
#define CANT_MAX_SERVIDORES 4

/*Numero maximo de puerto*/
#define NUM_MAX_PUERTO  65535

/*Puerto por default que el server escuchara*/
#define NUM_PUERTO_SERVIDOR_DEFECTO 80

#define MAX_BAUD_RATE 921600

typedef void(*functionPointerType)(void);

struct cmd{
	char const nombre[CANT_MAX_CARACT_NOMBRE_CMD+1];
	uint8_t const cantidad_parametros;
	functionPointerType ejecutar;
};

struct cmd_recibido {
	char nombre[CANT_MAX_CARACT_NOMBRE_CMD+1];
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	uint8_t cantidad_parametros_recibidos;
};


#endif /* CMD_DEFINICION_H_ */
