/*
 * cmd_basicos.cpp
 *
 *  Created on: 16 abr. 2019
 *      Author: felip
 */
#include "cmd_definicion.h"
#include "Arduino.h"
#include <string.h>

/*Comandos basicos para el manejo del modulo*/
void cmd_MIS(){
	/*MIS - Module Is Alive*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_MRS(){
	/*MRS - Module Reset*/
	ESP.restart();
	return;
}

void cmd_MUC(){
	/*Module UART Configuration*/
	//uint32_t baud_rate = atoi(comando_recibido.parametros[0]);
	uint32_t baud_rate = 0;
	Serial.print(baud_rate);
	Serial.print('\n');
	//if(!dentro_intervalo(baud_rate,9600,MAX_BAUD_RATE)){
		/*Baud rate fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	//}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	Serial.flush();
	Serial.end();
	delay(1);
	Serial.begin(baud_rate);
	Serial.setRxBufferSize(TAM_BUFFER_SERIAL);
	return;
}

void cmd_GFH(){
	/*GFH - Get Free Heap*/
	Serial.print(ESP.getFreeHeap(),DEC);
	Serial.print(CMD_TERMINATOR);
	return;
}

