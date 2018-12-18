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
#define CMD_NOT_FOUND ''
#define CMD_NO_PARAM ''

/*Cantidad maxima de parametros por comando*/
#define CANT_MAX_PARAMETROS_CMD 5

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define CANT_MAX_CARACT_PARAMETRO 64

/*Cantidad maxima de caracteres que puede contener el nombre del comando*/
#define CANT_MAX_CARACT_NOMBRE_CMD 3

/*Numero maximo de comandos admitidos*/
#define CANT_MAX_CMD 22


//extern void cmd_WFC(void);
//void cmd_WFS(void);




//char delimiter[] = ",";

typedef void(*functionPointerType)(void);
struct cmd{
	char const nombre[4];
	uint8_t const cantidad_parametros;
	functionPointerType ejecutar;
};

struct cmd_recibido {
	char nombre[4];
	char parametros[5][32];
	uint8_t cantidad_parametros_recibidos;
};





#endif /* CMD_DEFINICION_H_ */
