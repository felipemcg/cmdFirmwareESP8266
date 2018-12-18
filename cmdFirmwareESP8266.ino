#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>

#include "test.h"
#include "puerto_serial.h"
#include "operaciones_paquetes.h"
#include "cmd_definicion.h"

/*Considerar usar cliente_tcp.setNoDelay para desactivar el algoritmo de naggle*/

/*-------------------------DEFINE's--------------------------------*/
//#define sDebug

/*---------------------*/
/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define TIEMPO_MS_ESPERA_RESPUESTA_SERVIDOR 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define TIEMPO_MS_ESPERA_CONEXION_WIFI 20000

/*Maximum number of Bytes for a packet*/
#define TAM_MAX_PAQUETE_DATOS_TCP 512

/*Numero maximo de clientes que puede manejar el modulo*/
#define CANT_MAX_CLIENTES 4

/*Numero maximo de servidores que puede manejar el modulo*/
#define CANT_MAX_SERVIDORES 4

/*Numero maximo de puerto*/
#define NUM_MAX_PUERTO  65535

/*Puerto por default que el server escuchara*/
#define NUM_PUERTO_SERVIDOR_DEFECTO 80
/*-------------------------------------------------------------------*/


WiFiClient cliente_tcp[CANT_MAX_CLIENTES];

/*Declaracion de los buffers utilizados para recibir datos del servidor*/
char	buffer_datos_tcp_recibidos_servidor[CANT_MAX_CLIENTES][TAM_MAX_PAQUETE_DATOS_TCP+1];
uint16_t cant_bytes_recibidos_tcp_servidor[CANT_MAX_CLIENTES];
bool	b_buffer_servidor_tcp_lleno[CANT_MAX_CLIENTES];
/*Declaracion del objeto que se utilizara para el manejo del cliente, maximo 4
 * por limitacion del modulo.*/

/*Bandera para indicar que el servidor esta activo*/
bool	b_servidor_encendido[CANT_MAX_SERVIDORES] = {false,false,false,false};
uint16_t num_puerto_en_uso_servidor[CANT_MAX_SERVIDORES];
uint8_t cant_clientes_permitidos_en_servidor[CANT_MAX_SERVIDORES];
uint8_t cant_clientes_activos_en_servidor[CANT_MAX_SERVIDORES];
/*Declaracion de los objetos que se utilizaran para el manejo del servidor*/
std::vector<WiFiServer> server(CANT_MAX_SERVIDORES, WiFiServer(NUM_PUERTO_SERVIDOR_DEFECTO));




//#define sDebug 1
/*-------------------------------------------------------------------*/

const struct cmd conjunto_comandos[CANT_MAX_CMD] = {
  		{"WFC",2,&cmd_WFC}, //0
  		{"WFS",2,&cmd_WFS}, //1
		{"WRI",0,&cmd_WRI},	//2
		{"WID",0,&cmd_WID},	//3
		{"WFD",0,&cmd_WFD},	//4
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
		{"WFA",5,&cmd_WFA},	//16
  		{"WAC",3,&cmd_WAC}	//17
};

struct cmd_recibido comando_recibido;
char paquete_datos_tcp[TAM_MAX_PAQUETE_DATOS_TCP];

void setup() {
	Serial.setRxBufferSize(512);
	Serial.begin(115200);
	delay(10);
	Serial.println();
	for(int i=0; i < CANT_MAX_CLIENTES; i++){
		cant_bytes_recibidos_tcp_servidor[i] = 0;
		b_buffer_servidor_tcp_lleno[i] = false;
	}

#ifdef sDebug
	Serial.println();
    Serial.println();
    Serial.println("Listo.");
    Serial.println("Las instrucciones que tengo son: ");
    for(int i=0; i < CANT_MAX_CMD; i++){
  	  Serial.println((instructionSet[i]));
    }
#endif

    Serial.print("R");
    Serial.print(CMD_TERMINATOR);
    cliente_tcp[0].setNoDelay(1);
    cliente_tcp[1].setNoDelay(1);
    cliente_tcp[2].setNoDelay(1);
    cliente_tcp[3].setNoDelay(1);
    functionTest(4);
}

void loop() {
	uint8_t cant_maxima_caracteres_paquete_serial = CANT_MAX_CARACT_NOMBRE_CMD + CANT_MAX_CARACT_PARAMETRO*CANT_MAX_PARAMETROS_CMD + CANT_MAX_PARAMETROS_CMD;
	char paquete_serial[64];
	int indice_comando;

	comando_recibido.nombre[0] = '\0';
	comando_recibido.parametros[0][0] = '\0';
	comando_recibido.parametros[1][0] = '\0';
	comando_recibido.parametros[2][0] = '\0';
	comando_recibido.parametros[3][0] = '\0';
	comando_recibido.parametros[4][0] = '\0';

	yield();

	/*Se verifica que se haya recibido un nuevo paquete por el puerto serial.*/
	if (recibir_paquetes(paquete_serial,paquete_datos_tcp) == 1){
		//Serial.println(paquete_serial);
		yield();
		Serial.print("Antes de separar:");
		Serial.println(paquete_serial);

		/*Separar el paquete en los campos correspondientes.*/
		comando_recibido = separar_comando_parametros(paquete_serial);

		/*Se muestran los datos separados en campos.*/
		imprimir_datos_separados(comando_recibido);

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
	}else{
		/*Problemas en la recepcion de paquete serial*/
	}

	yield();

	/*Se almacenan los datos recibidos en los sockets en sus respectivos buffer's*/
	//receiveFromServer();

	yield();

	refreshServerClients();

	memset(paquete_datos_tcp,0,sizeof(paquete_datos_tcp));
}


/* Descripcion: Ejectua el comando solicitado.*/


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

/* Descripcion: Recibe un byte del servidor*/
void receiveFromServer(){
	uint16_t bytesAvailable;
	char dump, payload;
	for(int i = 0; i < CANT_MAX_CLIENTES; i++){
		if(cliente_tcp[i].connected()){
			/*Retorna la cantidad de bytes disponibles*/
			bytesAvailable = cliente_tcp[i].available();
			/*Si hay bytes disponibles y el buffer no esta lleno*/
			if (bytesAvailable) {
				if(!b_buffer_servidor_tcp_lleno[i]){
					//Serial.println("Leido");
					/*Lee el siguiente byte recibido*/
					payload = cliente_tcp[i].read();
					if(payload == -1){
						/*Serial.print("Ouch");
						Serial.print(CMD_TERMINATOR);*/
					}else{
						/*Serial.print(cant_bytes_recibidos_tcp_servidor[i]);
						Serial.print('-');
						Serial.print(payload);
						Serial.print('=');*/
						buffer_datos_tcp_recibidos_servidor[i][cant_bytes_recibidos_tcp_servidor[i]] = payload;
						//Serial.print(buffer_datos_tcp_recibidos_servidor[i][cant_bytes_recibidos_tcp_servidor[i]]);
						cant_bytes_recibidos_tcp_servidor[i]++;
					}
					/*Si la cantidad de bytes leidos por el servidor supero el tamaño
					 * del buffer, se indica que se lleno el buffer.*/
					if(cant_bytes_recibidos_tcp_servidor[i] > TAM_MAX_PAQUETE_DATOS_TCP){
						//Serial.println("Me llene");
						b_buffer_servidor_tcp_lleno[i] = true;
					}
				}else{
					//Serial.println("Dump");
					dump = cliente_tcp[i].read();

				}
			}
		}
	}
}

/* Descripcion: Verifica el numero val este dentro del rango limitado por min y max*/
bool inRange(uint16_t val, uint16_t min, uint16_t max)
{
  return ((min <= val) && (val <= max));
}

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
uint8_t acceptClients(WiFiServer& server, uint8_t serverId){
	uint8_t socket;
	/*Verificar que todavia no se conectaron el numero maximo de clientes*/
	if(cant_clientes_activos_en_servidor[serverId] < cant_clientes_permitidos_en_servidor[serverId]){
		socket = obtener_socket_libre();
		if(socket!=255){
			/*Se encontro socket libre*/
			/*Se acepta al cliente*/
			cliente_tcp[socket] = server.available();
			cant_clientes_activos_en_servidor[serverId]++;
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket);
			Serial.print(CMD_TERMINATOR);
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

void refreshServerClients(){
	/*Aca se tiene que verificar si los servidores tienen clientes en cola, para rechazarlos si es necesario por el backlog*/
	uint8_t i;
	for (i = 0; i < CANT_MAX_SERVIDORES; i++){
		if(server[i].hasClient() && (cant_clientes_activos_en_servidor[i] == cant_clientes_permitidos_en_servidor[i]) ){
			WiFiClient serverClient = server[i].available();
			serverClient.stop();
		}
	}
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


void cmd_WFC(){
	/*WFC - WiFi Connect*/
	unsigned long millis_anterior;
	unsigned long millis_actual;
	wl_status_t estado_conexion_wifi;
	bool b_conexion_wifi_timeout = 0;

	WiFi.mode(WIFI_STA);
	WiFi.begin(comando_recibido.parametros[0],comando_recibido.parametros[1]);
	millis_anterior = millis();
	estado_conexion_wifi = WiFi.status();
	while (estado_conexion_wifi != WL_CONNECTED) {
		millis_actual = millis();
		if((millis_actual - millis_anterior) > TIEMPO_MS_ESPERA_CONEXION_WIFI) {
			b_conexion_wifi_timeout = 1;
			break;
		}
		estado_conexion_wifi = WiFi.status();
		delay(20);
	}
	if(b_conexion_wifi_timeout == 0){
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_TERMINATOR);
		return;
	}else{
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	switch(estado_conexion_wifi){
		case WL_IDLE_STATUS:
			Serial.print("7");
			break;
		case WL_NO_SSID_AVAIL:
			Serial.print("4");
			break;
		case WL_SCAN_COMPLETED:
			Serial.print("5");
			break;
		case WL_CONNECT_FAILED:
			/*Significa que la contraseña es incorrecta*/
			Serial.print("3");
			break;
		case WL_CONNECTION_LOST:
			Serial.print("6");
			break;
		case WL_DISCONNECTED:
			Serial.print("1");
			break;
		case WL_CONNECTED:
			Serial.print(CMD_RESP_OK);
			break;
		default:
			break;
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_MIS(){
	/*MIS - Module Is Alive*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_CCS(){
	/*CCS - cliente_tcp Connect to Server*/

	uint16_t puerto_tcp;
	uint8_t socket;
	int C_STATUS = 0;
	wl_status_t estado_conexion_wifi;

	puerto_tcp = atoi(comando_recibido.parametros[1]);
	if(!inRange(puerto_tcp,0,NUM_MAX_PUERTO)){
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
		cliente_tcp[socket].setNoDelay(1);
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

void cmd_WFS(){
	/*WFS - WiFi Scan*/
	int cant_punto_acceso_encontrados;
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);
	cant_punto_acceso_encontrados = WiFi.scanNetworks();
	if (cant_punto_acceso_encontrados == -1) {
		Serial.print("1");
	}else{
		// print the list of networks seen:
		Serial.print(cant_punto_acceso_encontrados);

		// print the network number and name for each network found:
		for (int thisNet = 0; thisNet < cant_punto_acceso_encontrados; thisNet++) {
			Serial.print(thisNet);
			Serial.print(") ");
			Serial.print(WiFi.SSID(thisNet));
		//Serial.printf("SSID: %s", WiFi.SSID().c_str());
			Serial.print("\tSignal: ");
			Serial.print(WiFi.RSSI(thisNet));
			Serial.print(" dBm");
		}
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WRI(){
	/*WRI - WiFi RSSI*/
	Serial.print(WiFi.RSSI());
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WID(){
	Serial.print(WiFi.SSID());
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WFD(){
	/*WFD - WiFi Disconnect*/
	WiFi.disconnect();
	delay(100);
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WCF(){
	IPAddress ip,dns,gateway,subnet;
	if(!ip.fromString(comando_recibido.parametros[0])){
		/*IP Invalida*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dns.fromString(comando_recibido.parametros[1])){
		/*DNS Invalido*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(comando_recibido.parametros[2])){
		/*Gateway Invalido*/
		Serial.print("3");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(comando_recibido.parametros[3])){
		/*Subnet Invalido*/
		Serial.print("4");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(WiFi.config(ip,gateway,subnet,dns)){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print("5");
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SLC(){
	/*Agregar el parametro de cuantos clientes puede aceptar*/
	/*SLC - Server Listen to Clients*/
	wl_status_t estado_conexion_wifi;
	uint8_t i;
	uint8_t backlog;
	uint16_t puerto_tcp;
	bool b_puerto_tcp_en_uso = false;

	puerto_tcp = atoi(comando_recibido.parametros[0]);
	backlog = atoi(comando_recibido.parametros[1]);
	/*Determinar primero si el puerto es valido*/
	if(!inRange(puerto_tcp,0,NUM_MAX_PUERTO)){
		/*El numero de puerto esta fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!inRange(backlog,0,CANT_MAX_CLIENTES)){
		/*El numero de clientes esta fuera de rango*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}else{
		/*Verificar que se tienen los recursos disponibles para escuchar la cantidad de clientes*/
	}
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*Determinar si ya existe un servidor funcionando con ese puerto*/
	for (i = 0; i < CANT_MAX_SERVIDORES; i++) {
		if(puerto_tcp != num_puerto_en_uso_servidor[i]){
			b_puerto_tcp_en_uso = false;
			break;
		}else{
			b_puerto_tcp_en_uso = true;
			break;
		}
	}
	/*Si no existe un servidor con ese puerto, determinar que servidor esta libre y crear el servidor*/
	if(!b_puerto_tcp_en_uso){
		for (i = 0; i < CANT_MAX_SERVIDORES; i++) {
			if(server[i].status() == CLOSED){
				num_puerto_en_uso_servidor[i] = puerto_tcp;
				cant_clientes_permitidos_en_servidor[i] = backlog;
				server[i].begin(puerto_tcp);
				Serial.print(CMD_RESP_OK);
				Serial.print(CMD_DELIMITER);
				Serial.print(i,DEC);
				Serial.print(CMD_TERMINATOR);
				b_servidor_encendido[i] = true;
				break;
			}
		}
	}else{
		/*Indica que ya existe un servidor escuchando en ese puerto*/
		Serial.print("3");
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

void cmd_SAC(){
	/*SAC - Server Accept Clients*/

	uint8_t socket;
	wl_status_t estado_conexion_wifi;
	uint8_t serverAcceptStatus;

	socket = atoi(comando_recibido.parametros[0]);
	if(!inRange(socket,0,CANT_MAX_SERVIDORES)){
		/*El numero de socket esta fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print('6');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(b_servidor_encendido[socket]){
		if(server[socket].hasClient()){
			serverAcceptStatus = acceptClients(server[socket],socket);
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
	if(!inRange(socket,0,CANT_MAX_SERVIDORES)){
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
	server[socket].stop();
	b_servidor_encendido[socket] = false;
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SOW(){
	/*SOW - Socket Write*/
	/*Verificar primero si existe una conexion activa antes de intentar enviar el mensaje*/
	Serial.println("Llegue a SOW:");
	uint8_t socket = 255;
	int cant_bytes_enviar_tcp = -1;
	int cant_bytes_enviados_tcp = -2;
	wl_status_t estado_conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	cant_bytes_enviar_tcp = atoi(comando_recibido.parametros[1]);
	estado_conexion_wifi = WiFi.status();
	if( (estado_conexion_wifi == WL_DISCONNECTED) || (estado_conexion_wifi == WL_CONNECTION_LOST) ){
		/*WiFi desconectado*/
		Serial.print('5');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(inRange(socket,0,CANT_MAX_CLIENTES) == true){
		if(inRange(cant_bytes_enviar_tcp,0,TAM_MAX_PAQUETE_DATOS_TCP) == true){
			if(cliente_tcp[socket].connected()){
				/*data to print: char, byte, int, long, or string*/
				/*The max packet size in TCP is 1460 bytes*/


				 /* ======>  comando_recibido.parametros[2] solo puede ser uint8_t ??*/
				/*cliente_tcp.write() blocks until either data is sent and ACKed, or timeout occurs (currently hard-coded to 5 seconds)
				 * @ https://github.com/esp8266/Arduino/issues/917#issuecomment-150304010*/
				Serial.print("Voy a enviar: ");
				Serial.println(paquete_datos_tcp);
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
	if(!inRange(socket,0,CANT_MAX_CLIENTES)){
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
	//Serial.print(buffer_datos_tcp_recibidos_servidor[socket]);
	/*Revisar para llamar a la funcion correcta (la que recibe size como parametro)*/
	//while
	//Serial.write((const char*)buffer_datos_tcp_recibidos_servidor[socket],(uint16_t)cant_bytes_recibidos_tcp_servidor[socket]);
	Serial.print(CMD_TERMINATOR);
	cant_bytes_recibidos_tcp_servidor[socket] = 0;
	/*Clear the buffer*/
	for (int i=0; i < TAM_MAX_PAQUETE_DATOS_TCP; i++){
		buffer_datos_tcp_recibidos_servidor[socket][i] = 0;
	}
	b_buffer_servidor_tcp_lleno[socket] = false;

	return;
}

void cmd_SOC(){
	/*SOC- Socket Close*/
	uint8_t socket;
	wl_status_t estado_conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	if(!inRange(socket,0,CANT_MAX_CLIENTES) == true){
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

void cmd_GFH(){
	/*GFH - Get Free Heap*/
	Serial.print(ESP.getFreeHeap(),DEC);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WFA(){
	/*WFA - WiFi Soft-AP Mode*/
	bool b_punto_acceso_creado = 0;
	uint8_t canal_wifi = 1;
	uint8_t hidden_opt = 0;
	uint8_t cant_max_clientes_wifi = 4;

	canal_wifi = atoi(comando_recibido.parametros[2]);
	hidden_opt = atoi(comando_recibido.parametros[3]);
	cant_max_clientes_wifi = atoi(comando_recibido.parametros[4]);

	if(!inRange(canal_wifi,1,13)){
		/*El numero de canal_wifi esta fuera de rango*/
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!inRange(hidden_opt,0,1)){
		/*El numero de hidden_opt esta fuera de rango*/
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!inRange(cant_max_clientes_wifi,1,4)){
		/*El numero de cant_max_clientes_wifi esta fuera de rango*/
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	b_punto_acceso_creado = WiFi.softAP(comando_recibido.parametros[0], comando_recibido.parametros[1], canal_wifi, hidden_opt, cant_max_clientes_wifi);
	if(b_punto_acceso_creado == 0){
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WAC(){
	/*WFA - WiFi Soft-AP Mode*/
	bool b_configuracion_punto_acceso = 0;
	IPAddress ip_local;
	IPAddress gateway;
	IPAddress subnet;
	if(!ip_local.fromString(comando_recibido.parametros[0])){
		/*IP local Invalido*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(comando_recibido.parametros[1])){
		/*Gateway Invalido*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(comando_recibido.parametros[2])){
		/*Subnet Invalido*/
		Serial.print("3");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	b_configuracion_punto_acceso = WiFi.softAPConfig(ip_local, gateway, subnet);
	if(b_configuracion_punto_acceso == 0){
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;

}
