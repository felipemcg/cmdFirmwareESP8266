/*
 * cmd_tcp.cpp
 *
 *  Created on: 16 abr. 2019
 *      Author: felip
 */
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "cmd_definicion.h"
#include "utilidades.h"

/*Declaracion del objeto que se utilizara para el manejo del cliente, maximo 4
 * por limitacion del modulo.*/
WiFiClient cliente_tcp[CANT_MAX_CLIENTES];

std::vector<WiFiServer> servidor_obj(CANT_MAX_SERVIDORES, WiFiServer(NUM_PUERTO_SERVIDOR_DEFECTO));
struct elementos_servidor{
	bool b_activo;
	uint16_t num_puerto_en_uso;
	uint8_t cant_maxima_clientes_permitidos;
	uint8_t cant_clientes_activos;
};
struct elementos_servidor servidor[CANT_MAX_SERVIDORES];

typedef enum
{ TIPO_SERVIDOR, TIPO_CLIENTE, NINGUNO } tipoSocket;

struct sock_info
{
	tipoSocket  tipo;
	int8_t indice_servidor;
};

sock_info socket_info[CANT_MAX_CLIENTES];

size_t tam_buffer_serial = 0;

WiFiMode_t modo_wifi;

/* Descripcion: Devuelve el primer indice del socket dispoinible*/
uint8_t	obtener_socket_libre(){
	uint8_t i=0;
	for (i = 0; i < CANT_MAX_CLIENTES; i++) {
		/*Si esta vacio y no esta conectado*/
		if (!cliente_tcp[i] || !cliente_tcp[i].connected()) {
			if (cliente_tcp[i]) {
			  cliente_tcp[i].stop();
			}
			return i;
		}
	}
	/*Si no hay ningun lugar disponible*/
	if (i == CANT_MAX_CLIENTES) {
		return 255;
	}
	return 255;
}

/* Descripcion: Se encarga de aceptar a clientes que se quieren conectar al servidor*/
uint8_t servidor_acepta_clientes(WiFiServer& server, uint8_t serverId){
	uint8_t socket;
	/*Verificar que todavia no se conectaron el numero maximo de clientes*/
	if(servidor[serverId].cant_clientes_activos < servidor[serverId].cant_maxima_clientes_permitidos){
		socket = obtener_socket_libre();
		if(socket!=255){
			/*Se encontro socket libre*/
			/*Se acepta al cliente*/
			cliente_tcp[socket] = server.available();
			servidor[serverId].cant_clientes_activos++;
			socket_info[socket].tipo = TIPO_SERVIDOR;
			socket_info[socket].indice_servidor = serverId;
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket);
			return 1;
		}else{
			/*No hay socket disponible*/
			WiFiClient serverClient = server.available();
			serverClient.stop();
			return 2;
		}
	}else{
		/*No se admiten mas clientes para este servidor*/
		WiFiClient serverClient = server.available();
		serverClient.stop();
		return 3;
	}
	return 255;
}

void servidor_verificar_backlog(){
	/*Aca se tiene que verificar si los servidores tienen clientes en cola, para rechazarlos si es necesario por el backlog*/
	uint8_t i;
	for (i = 0; i < CANT_MAX_SERVIDORES; i++){
		if(servidor_obj[i].hasClient() && (servidor[i].cant_clientes_activos == servidor[i].cant_maxima_clientes_permitidos) ){
			WiFiClient serverClient = servidor_obj[i].available();
			serverClient.stop();
		}
	}
}

/*Comandos para el manejo de las operaciones TCP*/
void cmd_CCS(){
	/*CCS - cliente_tcp Connect to Server*/
	uint16_t puerto_tcp;
	uint8_t socket;
	int C_STATUS = 0;
	wl_status_t estado_conexion_wifi;

	puerto_tcp = atoi(comando_recibido.parametros[1]);
	if(!dentro_intervalo(puerto_tcp,0,NUM_MAX_PUERTO)){
		/*Puerto fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	socket = obtener_socket_libre();
	if(socket == 255){
		/*No hay socket disponible*/
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	C_STATUS = cliente_tcp[socket].connect(comando_recibido.parametros[0],puerto_tcp);
	if(C_STATUS){
		cliente_tcp[socket].setNoDelay(false);
		socket_info[socket].tipo = TIPO_CLIENTE;
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(socket);
		Serial.print(CMD_TERMINATOR);
	}else{
		/*No se pudo conectar*/
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

void cmd_SLC(){
	/*SLC - Server Listen to Clients*/
	wl_status_t estado_conexion_wifi;
	uint8_t i;
	uint8_t backlog;
	uint16_t puerto_tcp;
	bool b_puerto_tcp_en_uso = false;

	puerto_tcp = atoi(comando_recibido.parametros[0]);
	backlog = atoi(comando_recibido.parametros[1]);
	/*Determinar primero si el puerto es valido*/
	if(!dentro_intervalo(puerto_tcp,0,NUM_MAX_PUERTO)){
		/*El numero de puerto esta fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(backlog,0,CANT_MAX_CLIENTES)){
		/*El numero de clientes esta fuera de rango*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}else{
		/*Verificar que se tienen los recursos disponibles para escuchar la cantidad de clientes*/
	}
	/*estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		WiFi desconectado
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
		return;
	}*/
	/*Determinar si ya existe un servidor funcionando con ese puerto*/
	for (i = 0; i < CANT_MAX_SERVIDORES; i++) {
		if(puerto_tcp != servidor[i].num_puerto_en_uso){
			b_puerto_tcp_en_uso = false;
			break;
		}else{
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(i,DEC);
			Serial.print(CMD_TERMINATOR);
			return;
		}
	}
	/*Si no existe un servidor con ese puerto, determinar que servidor esta libre y crear el servidor*/
	if(!b_puerto_tcp_en_uso){
		for (i = 0; i < CANT_MAX_SERVIDORES; i++) {
			if(servidor_obj[i].status() == CLOSED){
				servidor[i].num_puerto_en_uso = puerto_tcp;
				servidor[i].cant_maxima_clientes_permitidos= backlog;
				servidor_obj[i].begin(puerto_tcp);
				Serial.print(CMD_RESP_OK);
				Serial.print(CMD_DELIMITER);
				Serial.print(i,DEC);
				Serial.print(CMD_TERMINATOR);
				servidor[i].b_activo = true;
				break;
			}
		}
	}
	return;
}

void cmd_SAC(){
	/*SAC - Server Accept Clients*/

	uint8_t socket;
	wl_status_t estado_conexion_wifi;
	uint8_t serverAcceptStatus;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_SERVIDORES)){
		/*El numero de socket esta fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		WiFi desconectado
		Serial.print('6');
		Serial.print(CMD_TERMINATOR);
		return;
	}*/
	if(servidor[socket].b_activo){
		if(servidor_obj[socket].hasClient()){
			serverAcceptStatus = servidor_acepta_clientes(servidor_obj[socket],socket);
			if(serverAcceptStatus == 1){
				/*Hay socket disponible*/
			}
			if(serverAcceptStatus == 2){
				/*No hay socket disponible*/
				Serial.print("2");
			}
			if(serverAcceptStatus == 3){
				/*No se aceptan mas clientes en este servidor*/
				Serial.print("3");
			}
		}else{
			/*El servidor no tiene clientes*/
			Serial.print("4");
		}
	}else{
		/*El servidor se encuentra desactivado*/
		Serial.print("5");
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SCC(){
	/*SCC - Server Close Connection*/
	uint8_t socket;
	wl_status_t estado_conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_SERVIDORES)){
		/*El numero de socket esta fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	servidor_obj[socket].stop();
	servidor[socket].b_activo = false;
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SOW(){
	/*SOW - Socket Write*/
	/*Verificar primero si existe una conexion activa antes de intentar enviar el mensaje*/
	//Serial.println("Llegue a SOW:");
	uint8_t socket = 255;
	int cant_bytes_enviar_tcp = -1;
	int cant_bytes_enviados_tcp = -2;
	wl_status_t estado_conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	cant_bytes_enviar_tcp = atoi(comando_recibido.parametros[1]);
	/*estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		WiFi desconectado
		Serial.print('5');
		Serial.print(CMD_TERMINATOR);
		return;
	}*/
	if(dentro_intervalo(socket,0,CANT_MAX_CLIENTES) == true){
		if(dentro_intervalo(cant_bytes_enviar_tcp,0,TAM_MAX_PAQUETE_DATOS_TCP) == true){
			if(cliente_tcp[socket].connected()){
				/*data to print: char, byte, int, long, or string*/
				/*The max packet size in TCP is 1460 bytes*/


				 /* ======>  comando_recibido.parametros[2] solo puede ser uint8_t ??*/
				/*cliente_tcp.write() blocks until either data is sent and ACKed, or timeout occurs (currently hard-coded to 5 seconds)
				 * @ https://github.com/esp8266/Arduino/issues/917#issuecomment-150304010*/
				/*Serial.print("Cant bytes: ");
				Serial.println(cant_bytes_enviar_tcp,DEC);
				Serial.print("Voy a enviar: ");
				Serial.println(paquete_datos_tcp);*/
				cant_bytes_enviados_tcp = cliente_tcp[socket].write(paquete_datos_tcp,cant_bytes_enviar_tcp);

				if(cant_bytes_enviar_tcp != cant_bytes_enviados_tcp){
					/*No se pudo escribir los datos al socket*/
					Serial.print("1");
					Serial.print(CMD_TERMINATOR);
				}else{
					/*Los datos se enviaron correctamente*/
					Serial.print(CMD_RESP_OK);
					Serial.print(CMD_TERMINATOR);
				}
			}else{
				/*El socket no esta conectado*/
				Serial.print("2");
				Serial.print(CMD_TERMINATOR);
			}
		}else{
			/*Numero de bytes para escribir fuera de rango*/
			Serial.print("3");
			Serial.print(CMD_TERMINATOR);
		}
	}else{
		/*Numero de socket fuera de rango*/
		Serial.print("4");
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

void cmd_SOR(){
	/*SOR - Socket Read*/
	uint8_t socket;
	wl_status_t estado_conexion_wifi;
	uint16_t bytes_disponible_para_recibir = 0;

	socket = atoi(comando_recibido.parametros[0]);
	/*print received data from server*/
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES)){
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		WiFi desconectado
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}*/
	/*Agregar verificacion de conexion a servidor antes de imprimir respuesta*/
	if(!cliente_tcp[socket].connected()){
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	bytes_disponible_para_recibir = cliente_tcp[socket].available();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(bytes_disponible_para_recibir,DEC);
	Serial.print(CMD_DELIMITER);
	while(bytes_disponible_para_recibir--){
		Serial.print((char)cliente_tcp[socket].read());
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SOC(){
	/*SOC- Socket Close*/
	uint8_t socket;
	wl_status_t estado_conexion_wifi;
	uint8_t server_id;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES) == true){
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	cliente_tcp[socket].stop();
	if(socket_info[socket].tipo == TIPO_SERVIDOR)
	{
		server_id = socket_info[socket].indice_servidor;
		servidor[server_id].cant_clientes_activos--;
		socket_info[socket].indice_servidor = -1;
	}
	socket_info[socket].tipo = NINGUNO;
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SRC(){
	uint8_t socket;
	socket = atoi(comando_recibido.parametros[0]);
	if (cliente_tcp[socket]) {
	  cliente_tcp[socket].stop();
	}
	Serial.println("Closed.");
	return;
}

void cmd_WFM(){
	uint8_t parametro_modo_wifi;
	WiFiMode_t modo_wifi;
	bool ret_val;
	parametro_modo_wifi = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(parametro_modo_wifi,0,3) == true){
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	switch (parametro_modo_wifi) {
		case 0:
			modo_wifi = WIFI_OFF;
			break;
		case 1:
			modo_wifi = WIFI_STA;
			break;
		case 2:
			modo_wifi = WIFI_AP;
			break;
		case 3:
			modo_wifi = WIFI_AP_STA;
			break;
		default:
			break;
	}
	ret_val = WiFi.mode(modo_wifi);
	if(ret_val == true){
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_TERMINATOR);
	}else{
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
	}
	return;
}





