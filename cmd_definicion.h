/*
 * cmd_definicion.h
 *
 *  Created on: 19 nov. 2018
 *      Author: felip
 */

#ifndef CMD_DEFINICION_H_
#define CMD_DEFINICION_H_


#define CMD_TERMINATOR '\n'
#define CMD_DELIMITER ','
#define CMD_RESP_OK '0'
#define CMD_NOT_FOUND 'C'
#define CMD_NO_PARAM 'P'

/*Cantidad maxima de parametros por comando*/
#define CANT_MAX_PARAMETROS_CMD 5

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define CANT_MAX_CARACT_PARAMETRO 64

/*Cantidad maxima de caracteres que puede contener el nombre del comando*/
#define CANT_MAX_CARACT_NOMBRE_CMD 3

/*Numero maximo de comandos admitidos*/
#define CANT_MAX_CMD 34

/*Cantidad maxima de parametros variables que puede permitir un comando*/
#define CANT_MAX_PARAM_VARIABLES 2

#define CMD_ERROR_1 '1'
#define CMD_ERROR_2 '2'
#define CMD_ERROR_3 '3'
#define CMD_ERROR_4 '4'
#define CMD_ERROR_5 '5'
#define CMD_ERROR_6 '6'
#define CMD_ERROR_7 '7'
#define CMD_ERROR_8 '8'
#define CMD_ERROR_9 '9'
#define CMD_ERROR_10 '10'

//char delimiter[] = ",";

typedef void(*functionPointerType)(void);
struct cmd{
	char const nombre[4];
	uint8_t const cantidad_parametros[CANT_MAX_PARAM_VARIABLES];
	functionPointerType ejecutar;
};

struct cmd_recibido {
	char nombre[4];
	char parametros[5][32];
	uint8_t cantidad_parametros_recibidos;
};





#endif /* CMD_DEFINICION_H_ */
