#include "Arduino.h"
#include <string.h>

/* TODO: Cambiar el retorno de esta funcion, de manera tal a que proporcione mas
 * detalles acerca de los errores. Por ejemplo, cambiar a int8_t y retornar los
 * errores negativos.*/
bool recibir_paquete(char *paquete_serial){
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
	while (Serial.available() > 0 && b_paquete_serial_recibido == false) {
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
			//Agregar verificacion de tamaño de paquete.
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
                paquete_serial[numero_caracter_recibido] = caracter_recibido;
                cantidad_caracteres_paquete_datos_tcp++;
                numero_caracter_recibido++;
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
