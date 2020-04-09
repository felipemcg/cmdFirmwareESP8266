/*
 * operaciones_paquete.cpp
 *
 *  Created on: 19 nov. 2018
 *      Author: felip
 */
#include "Arduino.h"
#include <string.h>
#include "cmd_definicion.h"

#define DEBUG_COMANDO_DATOS 0
#define DEBUG_TCP_BYTES 0
#define DEBUG_COMANDO 0
#define TIEMPO_MS_ESPERA_CARACTER 4000

char delimiter[2] = ",";

static unsigned long tiempo_actual_caracter_recibido = 0;
static unsigned long tiempo_anterior_caracter_recibido = 0;
typedef enum  {puerto_serial_recibiendo, puerto_serial_inactivo} status_puerto_serial;
static status_puerto_serial estado_puerto_serial = puerto_serial_inactivo;

bool recibir_paquetes(char *paquete_serial, char *paquete_datos_recibidos_tcp){
	bool b_ret_val = false;
	static bool b_par_coma = false;
	static bool b_tam_paquete_recibido = 0;
	static bool b_cmd_enviar_datos = 0;

	static uint16_t numero_caracter_recibido = 0;
	static uint16_t cantidad_comas = 0;
	static uint8_t pos_primera_coma = 0;
	static uint8_t pos_segunda_coma = 0;
	static uint8_t indice_buffer_tam_paquete_datos = 0;


	static uint16_t cantidad_caracteres_paquete_datos_tcp = 0;

	static uint8_t posicion_comienzo_datos = 0;
	static uint16_t tam_paquete_datos_tcp = 0;

	char caracter_recibido;
	char dump;
	static char buffer_comando[CANT_MAX_CARACT_NOMBRE_CMD + 1];
	static char buffer_tam_paquete_datos[5];

	tiempo_actual_caracter_recibido = millis();
	/*Se verifica que no haya ocurrido un TIMEOUT en la recepcion de los datos serial.*/
	if( ((tiempo_actual_caracter_recibido - tiempo_anterior_caracter_recibido) > TIEMPO_MS_ESPERA_CARACTER) && (estado_puerto_serial == puerto_serial_recibiendo)){
		/*Se vuelve a poner el puerto serial a un estado inactivo, y a la espera de un nuevo comando.*/
		estado_puerto_serial = puerto_serial_inactivo;
		/*Limpiar las variables utilizadas para los comandos comunes*/
		paquete_serial[0] = '\0';
		/*Limpiar las variables utilizados para los comandos de datos.*/
		b_par_coma = false;
		b_tam_paquete_recibido = false;
		b_cmd_enviar_datos = false;
		cantidad_comas = 0;
		pos_primera_coma = 0;
		pos_segunda_coma = 0;
		indice_buffer_tam_paquete_datos = 0;
		cantidad_caracteres_paquete_datos_tcp = 0;
		tam_paquete_datos_tcp = 0;
		memset(buffer_comando,0,sizeof(buffer_comando));
		memset(buffer_tam_paquete_datos,0,sizeof(buffer_tam_paquete_datos));
		posicion_comienzo_datos = pos_segunda_coma + 1;
		numero_caracter_recibido = 0;
		b_ret_val = false;
		Serial.print('T');
		Serial.print(CMD_TERMINATOR);
	}

	/*Se verifica que hayan datos para leer del buffer serial.*/
	if(Serial.available()){
		tiempo_anterior_caracter_recibido = millis();
		estado_puerto_serial = puerto_serial_recibiendo;

		/*Se lee un byte del buffer.*/
		caracter_recibido = Serial.read();

		/*Se verifica que no hubo error en la lectura*/
		if(caracter_recibido == -1){
			Serial.println("FAIL");
			b_ret_val = false;
		}else{
			if(numero_caracter_recibido < 3){
				buffer_comando[numero_caracter_recibido] = caracter_recibido;
			}
			//Se pregunta si se recibio el comando de datos(SOW)
			if(numero_caracter_recibido == 2){

	#if DEBUG_COMANDO
				Serial.print("Comando:");
				Serial.println(buffer_comando);
	#endif
				buffer_comando[3] = '\0';
				if( (!strcmp(buffer_comando,"SOW")) || (!strcmp(buffer_comando,"SDU"))  ){
	#if DEBUG_COMANDO_DATOS
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
	#if DEBUG
					Serial.print("Posicion primera coma:");
					Serial.println(pos_primera_coma);
	#endif
				}
				if(cantidad_comas == 2){
					pos_segunda_coma = numero_caracter_recibido;
	#if DEBUG
					Serial.print("Posicion segunda coma:");
					Serial.println(pos_segunda_coma);
	#endif
					b_par_coma = true;
				}
				cantidad_comas++;
			}

			/*Si se trata de recepcion de datos, se almacena la cantidad de bytes a recibir
			Agregar verificacion de si lo que se esta guardando es efectivamente un numero*/
			if( (b_cmd_enviar_datos == true) &&  (caracter_recibido != ',') && (cantidad_comas  > 1) && ( cantidad_comas < 3) ){
				buffer_tam_paquete_datos[indice_buffer_tam_paquete_datos++] = caracter_recibido;
			}

			 //Se verifica que la cantidad de bytes este presente y sea un numero valido
			if( (b_cmd_enviar_datos == true) && (b_par_coma == true) && (b_tam_paquete_recibido == false)){
				buffer_tam_paquete_datos[indice_buffer_tam_paquete_datos] = '\0';
	#if DEBUG_COMANDO_DATOS
					Serial.print("Buffer tam paquete:");
					Serial.println(buffer_tam_paquete_datos);
	#endif
				tam_paquete_datos_tcp = atoi((char*)buffer_tam_paquete_datos);
				//Agregar verificacion de tamaño de paquete.
				b_tam_paquete_recibido = 1;
	#if DEBUG_COMANDO_DATOS
					Serial.print("Tam paquete:");
					Serial.println(tam_paquete_datos_tcp);
	#endif
			}

			//Se verifica si lo que se va a recibir es un comando de datos o un comando normal.
			if((b_cmd_enviar_datos == true)  && (b_tam_paquete_recibido == true) && (numero_caracter_recibido > pos_segunda_coma) ){
	#if DEBUG_COMANDO_DATOS
					Serial.print("Entro a la rutina de SOW");
	#endif
					//Serial.println("Tuhna");
				//Se va leer datos del comando SOW
				if( cantidad_caracteres_paquete_datos_tcp < tam_paquete_datos_tcp ){
					//Se guardan los datos TCP recibidos
	#if DEBUG_TCP_BYTES
					Serial.print("Byte ");
					Serial.print(cantidad_caracteres_paquete_datos_tcp);
					Serial.print(": ");
					Serial.println(caracter_recibido);
	#endif

					paquete_datos_recibidos_tcp[cantidad_caracteres_paquete_datos_tcp] = caracter_recibido;
					cantidad_caracteres_paquete_datos_tcp++;
					//paquete_serial[numero_caracter_recibido] = caracter_recibido;
					//numero_caracter_recibido++;
				}else{
					//Ya se alcanzo la cantidad de bytes especificada.
					if(caracter_recibido == '\n'){
						//Se encontro el terminador(\n)
	#if DEBUG_COMANDO_DATOS
						Serial.println("Se encontro LF al final, luego de los datos.");
	#endif
						paquete_serial[numero_caracter_recibido] = '\0';
						b_ret_val = true;
					}else{
						/*No se encontro el caracter terminador,informar
						error de formato en el paquete*/
	#if DEBUG_COMANDO_DATOS
						Serial.println("No se encontro LF al final, luego de los datos.");
	#endif
						Serial.print('5');
						Serial.print(CMD_TERMINATOR);
						paquete_serial[0] = '\0';
						numero_caracter_recibido = 0;
						b_ret_val = false;
					}
					//Se vacia el buffer serial, necesario para no procesar basura
					while (Serial.available() > 0){
						dump = Serial.read();
					}
					/*Limpiar las variables utilizados para los comandos de datos.*/
					b_par_coma = false;
					b_tam_paquete_recibido = false;
					b_cmd_enviar_datos = false;
					cantidad_comas = 0;
					pos_primera_coma = 0;
					pos_segunda_coma = 0;
					indice_buffer_tam_paquete_datos = 0;
					cantidad_caracteres_paquete_datos_tcp = 0;
					tam_paquete_datos_tcp = 0;
					memset(buffer_comando,0,sizeof(buffer_comando));
					memset(buffer_tam_paquete_datos,0,sizeof(buffer_tam_paquete_datos));
					posicion_comienzo_datos = pos_segunda_coma + 1;
					numero_caracter_recibido = 0;
					estado_puerto_serial = puerto_serial_inactivo;
				}
				//Serial.println(millis_actual - millis_anterior);
			}else{
				//Se leen los datos como siempre
				if(caracter_recibido == '\n'){
					//Se encontro el terminador(\n)
					paquete_serial[numero_caracter_recibido] = '\0';
					//Serial.println("Se encontro LF al final, luego de los datos.");
					numero_caracter_recibido = 0;
					estado_puerto_serial = puerto_serial_inactivo;
					b_ret_val = true;
				}else{
	#if DEBUG_COMANDO
					Serial.print("Byte ");
					Serial.print(numero_caracter_recibido);
					Serial.print(": ");
					Serial.println(caracter_recibido);
	#endif
					paquete_serial[numero_caracter_recibido] = caracter_recibido;
					numero_caracter_recibido++;
					b_ret_val = false;
				}
			}
		}
	}
	return b_ret_val;
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
		//Serial.print(strtokIndx);
		//Serial.print(" - ");
		cantidad_delimitadores_encontrados++;
		/*Obtiene la instruccion*/
		if(cantidad_delimitadores_encontrados == 1){
			strcpy(comando.nombre,strtokIndx);
		}
		/*Obtiene los parametros*/
		if(cantidad_delimitadores_encontrados > 1){
			strcpy(comando.parametros[cantidad_parametros_encontrados],strtokIndx);
			//Serial.println(comando.parametros[cantidad_parametros_encontrados]);
			cantidad_parametros_encontrados++;

		}
		strtokIndx = strtok(NULL, delimiter);
	}
	comando.cantidad_parametros_recibidos = cantidad_parametros_encontrados;
	return comando;
}




