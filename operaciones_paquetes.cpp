/*
 * operaciones_paquete.cpp
 *
 *  Created on: 19 nov. 2018
 *      Author: felip
 */
#include "Arduino.h"
#include <string.h>
#include "cmd_definicion.h"

#define DEBUG

char delimiter[2] = ",";


bool recibir_paquetes(char *paquete_serial, char *paquete_datos_recibidos_tcp){
	bool b_par_coma = false;
	bool b_tam_paquete_recibido = 0;
	bool b_paquete_serial_recibido = 0;
	bool b_cmd_enviar_datos = 0;

	uint16_t numero_caracter_recibido = 0;
	uint16_t cantidad_comas = 0;
	uint8_t pos_primera_coma = 0;
	uint8_t pos_segunda_coma = 0;
	uint8_t indice_buffer_tam_paquete_datos = 0;

	uint16_t cantidad_caracteres_paquete_datos_tcp = 0;

	uint8_t posicion_comienzo_datos = 0;
	uint16_t tam_paquete_datos_tcp = 0;

	char caracter_recibido;
	char dump;
	char buffer_comando[4] = {'\0'};
	char buffer_tam_paquete_datos[4] = {'\0'};

	/*Mientras haya datos serial disponibles*/
	while  (Serial.available() && b_paquete_serial_recibido == false) {
		/*See recibe un caracter*/
		caracter_recibido = Serial.read();

		/*Se alceman 3 caracteres, para guardar el comando*/
		if(numero_caracter_recibido < 3){
			buffer_comando[numero_caracter_recibido] = caracter_recibido;
		}

		/*Se pregunta si se recibio el comando de datos(SOW)*/
		if(numero_caracter_recibido == 2){
#ifdef DEBUG
			Serial.print("Comando:");
			Serial.println(buffer_comando);
#endif
			buffer_comando[3] = '\0';
			if(!strcmp(buffer_comando,"SOW")){
#ifdef DEBUG
				Serial.println("SOW encontrado");
#endif
				b_cmd_enviar_datos = true;
			}else{
				b_cmd_enviar_datos = false;
			}
		}

		/*Se lleva la cuenta de las comas y se guarda la posicion de la primera y segunda, despues
		de eso ya no entra aqui, solo cuando se termino de procesar el paquete.*/
		if( (b_cmd_enviar_datos == true) && (caracter_recibido == ',') && (b_par_coma == false)){
			if(cantidad_comas == 1){
				pos_primera_coma = numero_caracter_recibido;
#ifdef DEBUG
            	Serial.print("Posicion primera coma:");
				Serial.println(pos_primera_coma);
#endif
			}
			if(cantidad_comas == 2){
				pos_segunda_coma = numero_caracter_recibido;
#ifdef DEBUG
            	Serial.print("Posicion segunda coma:");
				Serial.println(pos_segunda_coma);
#endif
				b_par_coma = true;
			}
			cantidad_comas++;
		}

        /*Si se trata de recepcion de datos, se almacena la cantidad de bytes a recibir*/
        /*Agregar verificacion de si lo que se esta guardando es efectivamente un numero*/
		if( (b_cmd_enviar_datos == true) &&  (caracter_recibido != ',') && (cantidad_comas  > 1) && ( cantidad_comas < 3) ){
			buffer_tam_paquete_datos[indice_buffer_tam_paquete_datos++] = caracter_recibido;
		}

        /* Se verifica que la cantidad de bytes este presente y sea un numero valido*/
        if( (b_cmd_enviar_datos == true) && (b_par_coma == true) && (b_tam_paquete_recibido == false)){
            buffer_tam_paquete_datos[indice_buffer_tam_paquete_datos] = '\0';
#ifdef DEBUG
            	Serial.print("Buffer tam paquete:");
				Serial.println(buffer_tam_paquete_datos);
#endif
			tam_paquete_datos_tcp = atoi((char*)buffer_tam_paquete_datos);
			//Agregar verificacion de tama�o de paquete.
	        b_tam_paquete_recibido = 1;
#ifdef DEBUG
            	Serial.print("Tam paquete:");
				Serial.println(tam_paquete_datos_tcp);
#endif
		}

        /*Se verifica si lo que se va a recibir son datos o u otro comando*/
        if((b_cmd_enviar_datos == true)  && (b_tam_paquete_recibido == true) && (numero_caracter_recibido > pos_segunda_coma) ){
            /*Se va leer datos del comando SOW*/
            if(cantidad_caracteres_paquete_datos_tcp < tam_paquete_datos_tcp){
                /*Se guardan los datos TCP recibidos*/
            	paquete_datos_recibidos_tcp[cantidad_caracteres_paquete_datos_tcp] = caracter_recibido;
            	cantidad_caracteres_paquete_datos_tcp++;
                //paquete_serial[numero_caracter_recibido] = caracter_recibido;
                //numero_caracter_recibido++;
            }else{
                /*Ya se alcanzo la cantidad de bytes especificada*/
                if(caracter_recibido == '\n'){
                    /*Se encontro el terminador(\n)*/
                	paquete_serial[numero_caracter_recibido] = '\0';
                }else{
                    /*No se encontro el caracter terminador,informar
                    error de formato en el paquete*/
                	paquete_serial[0] = '\0';
                	return false;
                }
                /*Se vacia el buffer serial, necesario para no procesar basura*/
				while (Serial.available() > 0){
					dump = Serial.read();
				}
				b_paquete_serial_recibido  = true;
                posicion_comienzo_datos = pos_segunda_coma + 1;
#ifdef DEBUG
                Serial.println(paquete_serial);
#endif
                return true;
            }
        }else{
            /*Se leen los datos como siempre*/
            if(caracter_recibido == '\n'){
                /*Se encontro el terminador(\n)*/
            	paquete_serial[numero_caracter_recibido] = '\0';
                b_paquete_serial_recibido  = 1;
                return true;
            }else{
            	paquete_serial[numero_caracter_recibido] = caracter_recibido;
                numero_caracter_recibido++;
            }
        }
	}
	return false;
}





/* Descripcion: Separa los datos recibidos en campos*/
struct cmd_recibido separar_comando_parametros(char *paquete){
	struct cmd_recibido comando;

	uint8_t cantidad_delimitadores_encontrados = 0;
	uint8_t cantidad_parametros_encontrados = 0;


	/*Puntero para indicar donde se encuentra la cadena*/
	char * strtokIndx; // this is used by strtok() as an index

	/*Obtiene la primera parte, la instruccion*/
	strtokIndx = strtok(paquete,delimiter);

	/*Mientras no encuentre el fin de la cadena*/
	while(strtokIndx != NULL){
		Serial.print(strtokIndx);
		Serial.print(" - ");
		cantidad_delimitadores_encontrados++;
		/*Obtiene la instruccion*/
		if(cantidad_delimitadores_encontrados == 1){
			strcpy(comando.nombre,strtokIndx);
		}
		/*Obtiene los parametros*/
		if(cantidad_delimitadores_encontrados > 1){
			strcpy(comando.parametros[cantidad_parametros_encontrados],strtokIndx);
			Serial.println(comando.parametros[cantidad_parametros_encontrados]);
			cantidad_parametros_encontrados++;

		}
		strtokIndx = strtok(NULL, delimiter);
	}
	comando.cantidad_parametros_recibidos = cantidad_parametros_encontrados;
	return comando;
}




