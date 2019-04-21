/*
 * procesador_comandos.cpp
 *
 *  Created on: 16 abr. 2019
 *      Author: felip
 */
#include "Arduino.h"
#include "operaciones_paquetes.h"
#include "cmd_definicion.h"
#include "cmd_basicos.h"
#include "cmd_wifi.h"

const struct cmd conjunto_comandos[CANT_MAX_CMD] = {
  		{"WFC",2,&cmd_WFC}, //0/
  		{"WFS",0,&cmd_WFS}, //1
		{"WRI",0,&cmd_WRI},	//2
		{"WID",0,&cmd_WID},	//3
		{"WFI",0,&cmd_WFI},
		{"WFD",1,&cmd_WFD},	//4
		{"WCF",4,&cmd_WCF},	//5
		{"CCS",2,&cmd_CCS},	//6
		{"SOW",2,&cmd_SOW},	//7
		{"SOR",1,&cmd_SOR},	//8
		{"SOC",1,&cmd_SOC},	//9
		{"SLC",2,&cmd_SLC},	//10
		{"SCC",1,&cmd_SCC},	//11
		{"SAC",1,&cmd_SAC},	//12
		{"SRC",1,&cmd_SRC},	//13
		{"GFH",0,&cmd_GFH},	//14
		{"MIS",0,&cmd_MIS},	//15
		{"MRS",0,&cmd_MRS},
		{"MUC",1,&cmd_MUC},
		{"WFA",5,&cmd_WFA},	//16
  		{"WAC",3,&cmd_WAC},	//17
  		{"WAS",0,&cmd_WAS},	//18
  		{"WAD",1,&cmd_WAD},
		{"WFM",1,&cmd_WFM},
  		{"WAI",0,&cmd_WAI}	//20
};





char paquete_datos_tcp[TAM_MAX_PAQUETE_DATOS_TCP];
char paquete_serial[64];
struct cmd_recibido comando_recibido;


void imprimir_datos_separados(struct cmd_recibido comando){
	Serial.print("Comando:");
	Serial.println(comando.nombre);
	for(int i = 0 ; i < comando.cantidad_parametros_recibidos; i++){
		Serial.print("Parametro ");
		Serial.print(i+1);
		Serial.print(":");
		Serial.println(comando.parametros[i]);
	}
	return;
}


/*
 * Descripcion: Compara la cadena que contiene el campo de instruccion(INST) con el array
 * de instrucciones(instructionSet). Si encuentra una coincidencia, guarda el indice y la
 * funcion retorna 1.
 * */
int buscar_comando(char cmd_nombre[4]){
	uint8_t indice_conjunto_comandos;
 	for(indice_conjunto_comandos = 0; indice_conjunto_comandos < CANT_MAX_CMD; indice_conjunto_comandos++){
		if(strcmp(cmd_nombre,conjunto_comandos[indice_conjunto_comandos].nombre) == 0){
			return 	indice_conjunto_comandos;
		}
	}
	/*Si llega aqui, retornar con error*/
	return -1;
}

/* Descripcion: Comprueba que se hayan recibido la cantidad necesaria de parametros ejecutar el comando.*/
bool validar_cantidad_parametros(uint8_t indice_comando, uint8_t cantidad_parametros){

	if((conjunto_comandos[indice_comando].cantidad_parametros == cantidad_parametros)){
		return 1;
	}
	return 0;
}

void liberar_recursos(void){
	for (uint8_t indice_clientes = 0; indice_clientes < CANT_MAX_CLIENTES; ++indice_clientes) {
		cliente_tcp[indice_clientes].stop();
	}
	for (uint8_t indice_servidores = 0; indice_servidores < CANT_MAX_SERVIDORES; ++indice_servidores) {
		servidor_obj[indice_servidores].stop();
		servidor[indice_servidores].b_activo = false;
		servidor[indice_servidores].num_puerto_en_uso = 0;
		servidor[indice_servidores].cant_maxima_clientes_permitidos = 0;
		servidor[indice_servidores].cant_clientes_activos = 0;
	}
	return;
}


void obtener_parametros(char *parametros){
	memcpy(parametros,comando_recibido.parametros,sizeof(comando_recibido.parametros));
	return;
}

void cmd_procesador(void)
{
	uint8_t cant_maxima_caracteres_paquete_serial = CANT_MAX_CARACT_NOMBRE_CMD + CANT_MAX_CARACT_PARAMETRO*CANT_MAX_PARAMETROS_CMD + CANT_MAX_PARAMETROS_CMD;
	int indice_comando;
	comando_recibido.nombre[0] = '\0';
	comando_recibido.parametros[0][0] = '\0';
	comando_recibido.parametros[1][0] = '\0';
	comando_recibido.parametros[2][0] = '\0';
	comando_recibido.parametros[3][0] = '\0';
	comando_recibido.parametros[4][0] = '\0';

	yield();

	/*Se verifica que se haya recibido un nuevo paquete por el puerto serial.*/
	if (recibir_paquetes(paquete_serial, paquete_datos_tcp) == 1){
		/*Serial.print("Dir serial: ");
		Serial.printf("%p",paquete_serial);
		Serial.println();
		Serial.print("Dir datos: ");
		Serial.printf("%p",paquete_datos_tcp);*/

		//Serial.println(paquete_serial);
		yield();
		//Serial.print("Antes de separar:");
		//Serial.println(paquete_serial);

		/*Separar el paquete en los campos correspondientes.*/
		comando_recibido = separar_comando_parametros(paquete_serial);

		/*Se muestran los datos separados en campos.*/
		//imprimir_datos_separados(comando_recibido);

		/*Se busca el comando recibido dentro del conjunto de comandos.*/
		indice_comando = buscar_comando(comando_recibido.nombre);

		if(indice_comando != -1){
			/*Se verifica que se recibio la cantidad necesaria de parametros para ejectuar el comando.*/
			if( validar_cantidad_parametros(indice_comando, comando_recibido.cantidad_parametros_recibidos)){

				/*Se llama a la funcion que ejecutara las acciones correspondientes al comando.*/
				conjunto_comandos[indice_comando].ejecutar();

			}else{
				/*No se cuenta con la cantidad de parametros necesarios*/
			}
		}else{
			/*No se encontro el comando*/
		}
		memset(paquete_serial,0,sizeof(paquete_serial));
		memset(paquete_datos_tcp,0,sizeof(paquete_datos_tcp));
	}else{
		/*Problemas en la recepcion de paquete serial*/
	}

	yield();

	servidor_verificar_backlog();

	return;
}



