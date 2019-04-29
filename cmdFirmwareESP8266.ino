#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

//#include "test.h"
//#include "puerto_serial.h"
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
#define TAM_MAX_PAQUETE_DATOS_TCP 1460

#define TAM_MAX_PAQUETE_DATOS_UDP 255

/*Numero maximo de clientes que puede manejar el modulo*/
#define CANT_MAX_CLIENTES 4

/*Numero maximo de servidores que puede manejar el modulo*/
#define CANT_MAX_SERVIDORES 4

/*Numero maximo de puerto*/
#define NUM_MAX_PUERTO  65535

/*Puerto por default que el server escuchara*/
#define NUM_PUERTO_SERVIDOR_DEFECTO 80

#define MAX_BAUD_RATE 921600
/*-------------------------------------------------------------------*/


/*Declaracion del objeto que se utilizara para el manejo del cliente, maximo 4
 * por limitacion del modulo.*/
WiFiClient cliente_tcp[CANT_MAX_CLIENTES];

WiFiUDP Udp;

std::vector<WiFiServer> servidor_obj(CANT_MAX_SERVIDORES,
		WiFiServer(NUM_PUERTO_SERVIDOR_DEFECTO));

std::vector<WiFiUDP> udp_obj(CANT_MAX_SERVIDORES,
		WiFiUDP());
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

//#define sDebug 1
/*-------------------------------------------------------------------*/

bool dentro_intervalo(uint32_t val, uint32_t min, uint32_t max);
void cmd_WFC(void);
void cmd_MRS(void);
void cmd_MUC(void);
void cmd_RVU(void);
void cmd_WFS(void);
void cmd_WRI(void);
void cmd_WID(void);
void cmd_WFI(void);
void cmd_WFD(void);
void cmd_WCF(void);
void cmd_CCS(void);
void cmd_SOW(void);
void cmd_SOR(void);
void cmd_SDU(void);
void cmd_RVU(void);
void cmd_SOC(void);
void cmd_SLC(void);
void cmd_SCC(void);
void cmd_SAC(void);
void cmd_SRC(void);
void cmd_GFH(void);
void cmd_MIS(void);
void cmd_WFA(void);
void cmd_WAC(void);
void cmd_WAS(void);
void cmd_WAD(void);
void cmd_WFM(void);
void cmd_WAI(void);

const struct cmd conjunto_comandos[CANT_MAX_CMD] = {
  		{"WFC",{2,0},&cmd_WFC}, //0/
  		{"WFS",{0,0},&cmd_WFS}, //1
		{"WRI",{0,0},&cmd_WRI},	//2
		{"WID",{0,0},&cmd_WID},	//3
		{"WFI",{0,0},&cmd_WFI},
		{"WFD",{1,0},&cmd_WFD},	//4
		{"WCF",{4,0},&cmd_WCF},	//5
		{"CCS",{2,3},&cmd_CCS},	//6
		{"SOW",{2,0},&cmd_SOW},	//7
		{"SOR",{1,0},&cmd_SOR},	//8
		{"SDU",{2,0},&cmd_SDU},	//7
		{"RVU",{1,0},&cmd_RVU},	//7
		{"SOC",{1,0},&cmd_SOC},	//9
		{"SLC",{2,0},&cmd_SLC},	//10
		{"SCC",{1,0},&cmd_SCC},	//11
		{"SAC",{1,0},&cmd_SAC},	//12
		{"SRC",{1,0},&cmd_SRC},	//13
		{"GFH",{0,0},&cmd_GFH},	//14
		{"MIS",{0,0},&cmd_MIS},	//15
		{"MRS",{0,0},&cmd_MRS},
		{"MUC",{1,0},&cmd_MUC},
		{"WFA",{5,0},&cmd_WFA},	//16
  		{"WAC",{3,0},&cmd_WAC},	//17
  		{"WAS",{0,0},&cmd_WAS},	//18
  		{"WAD",{1,0},&cmd_WAD},
		{"WFM",{1,0},&cmd_WFM},
  		{"WAI",{0,0},&cmd_WAI}	//20
};

struct cmd_recibido comando_recibido;
char paquete_datos_tcp[TAM_MAX_PAQUETE_DATOS_TCP];
char paquete_serial[64];

size_t tam_buffer_serial = 0;

WiFiMode_t modo_wifi_actual;
wl_status_t estado_conexion_wifi_interfaz_sta_actual;


void setup() {
	Serial.begin(115200);
	Serial.setRxBufferSize(1024);
	delay(10);
	Serial.println();

#ifdef sDebug
	Serial.println();
    Serial.println();
    Serial.println("Listo.");
    Serial.println("Las instrucciones que tengo son: ");
    for(int i=0; i < CANT_MAX_CMD; i++){
  	  Serial.println((instructionSet[i]));
    }
#endif

    for (int indice_socket = 0; indice_socket < CANT_MAX_CLIENTES; ++indice_socket) {
		socket_info[indice_socket].tipo = NINGUNO;
		socket_info[indice_socket].indice_servidor = -1;
	}

    /*cliente_tcp[0].setNoDelay(1);
    cliente_tcp[1].setNoDelay(1);
    cliente_tcp[2].setNoDelay(1);
    cliente_tcp[3].setNoDelay(1);*/
    modo_wifi_actual = WiFi.getMode();
    estado_conexion_wifi_interfaz_sta_actual = WiFi.status();
    Serial.print("R");
    Serial.print(CMD_TERMINATOR);
}

void loop() {
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

/* Descripcion: Verifica el numero val este dentro del rango limitado por min y max*/
bool dentro_intervalo(uint32_t val, uint32_t min, uint32_t max)
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

	if( (conjunto_comandos[indice_comando].cantidad_parametros[0] == cantidad_parametros)
			||  (conjunto_comandos[indice_comando].cantidad_parametros[1] == cantidad_parametros)){
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

/* Se verifica si el ESP8266 se encuentra conectado a alguna estacion, o si
 * alguna estacion esta conectado a el.
 */
int8_t verificar_conexion_wifi()
{
	uint8_t cant_clientes_conectados_interfaz_softap = 0;
	int8_t ret_val = 0;
	modo_wifi_actual = WiFi.getMode();
	switch (modo_wifi_actual)
	{
		case WIFI_OFF:
			ret_val = -1;
			break;
		case WIFI_AP:
			cant_clientes_conectados_interfaz_softap = WiFi.softAPgetStationNum();
			if(cant_clientes_conectados_interfaz_softap == 0)
			{
				ret_val = -2;
			}
			break;
		case WIFI_STA:
			estado_conexion_wifi_interfaz_sta_actual = WiFi.status();
			if( (estado_conexion_wifi_interfaz_sta_actual == WL_DISCONNECTED)
				|| (estado_conexion_wifi_interfaz_sta_actual == WL_CONNECTION_LOST))
			{
				ret_val = -2;
			}
			break;
		case WIFI_AP_STA:
			cant_clientes_conectados_interfaz_softap = WiFi.softAPgetStationNum();
			estado_conexion_wifi_interfaz_sta_actual = WiFi.status();
			if( (estado_conexion_wifi_interfaz_sta_actual == WL_DISCONNECTED)
				|| (estado_conexion_wifi_interfaz_sta_actual == WL_CONNECTION_LOST)
				|| (cant_clientes_conectados_interfaz_softap == 0) )
			{
				ret_val = -2;
			}
			break;
		default:
			ret_val = -3;
			break;
	}
	return ret_val;
}

/*Comandos basicos para el manejo del modulo*/

/**
 *  Comando que verifica que el modulo se encuentra encendido.
 *
 *  Hace un simple Serial.print para verificar que el modulo esta funcionando.
 *
 *  @param Ninguno.
 *  @return 0\n
 */
void cmd_MIS(){
	/*MIS - Module Is Alive*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

/**
 *  Comando que reinicia el modulo ESP8266.
 *
 *  @param Ninguno.
 *  @return Nada.
 */
void cmd_MRS(){
	/*MRS - Module Reset*/
	ESP.restart();
	return;
}

/**
 *  Comando que modifica la velocidad de puerto UART.
 *
 *	El rango de velocidad permitido va desde 9600 hasta 921600
 *
 *  @param baud_rate nueva velocidad de operacion para el UART.
 *  @retval 0 Si no hay errores.
 *  @retval 1 Si el valor de velocidad se encuentra fuera de rango.
 */
void cmd_MUC(){
	/*Module UART Configuration*/
	uint32_t baud_rate = atoi(comando_recibido.parametros[0]);
	Serial.print(baud_rate);
	Serial.print('\n');
	if(!dentro_intervalo(baud_rate,9600,MAX_BAUD_RATE)){
		/*Baud rate fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	Serial.flush();
	Serial.end();
	delay(1);
	Serial.begin(baud_rate);
	Serial.setRxBufferSize(1024);
	return;
}

/**
 *  Comando que imprime la cantidad de RAM disponible en Bytes.
 *
 *  @param 	Ninguno.
 *  @return Bytes disponibles de RAM en el modulo.
 */
void cmd_GFH(){
	/*GFH - Get Free Heap*/
	Serial.print(ESP.getFreeHeap(),DEC);
	Serial.print(CMD_TERMINATOR);
	return;
}

/*Comandos para configuracion del WiFi*/

/**
 *  Comando que explora los puntos de acceso dentro del alcance del modulo.
 *
 *  @param 	Ninguno.
 *  @return 0,NRO_APS_DISPONIBLES,SSID1;RSSI1,SSID2;RSSI2,SSIDn;RSSIn\n
 */
void cmd_WFS(){
	/*WFS - WiFi Scan*/
	ESP8266WiFiScanClass WiFiScan;
	int cant_punto_acceso_encontrados;
	cant_punto_acceso_encontrados = WiFiScan.scanNetworks();
	if (cant_punto_acceso_encontrados == -1) {
		Serial.print("1");
	}else{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(cant_punto_acceso_encontrados);
		for (int esta_red = 0; esta_red < cant_punto_acceso_encontrados; esta_red++) {
			Serial.print(CMD_DELIMITER);
			Serial.print((WiFiScan.SSID(esta_red)));
			Serial.print(';');
			Serial.print(WiFiScan.RSSI(esta_red));
		}
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

/**
 * Comando para configurar de forma manual los parametros de la interfaz de red.
 *
 *  @param 	IP
 *  @param  DNS
 *  @param  Gateway
 *  @param  Subnet
 *  @retval	1\n Direccion IP invalida.
 *  @retval 2\n Direccion DNS invalida.
 *  @retval 3\n Direccion de gateway invalida.
 *  @retval 4\n Direccion de subnet invalida.
 */
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

/**
 * Comando para conectar el modulo a un punto de acceso.
 *
 *  @param 	SSID 		Del punto de acceso al cual se quiere conectar.
 *  @param 	Contraseña 	Del punto de acceso al cual se quiere conectar.
 *  @retval 1\n Si no se pudo conectar al punto de acceso.
 *  @retval 2\n Si se alcanzo el timeout para conectarse (20 segundos).
 *  @retval 3\n La contraseña es incorrecta.
 *  @retval 4\n El SSID no esta disponible.
 */
void cmd_WFC(){
	/*WFC - WiFi Connect*/
	unsigned long millis_anterior;
	unsigned long millis_actual;
	wl_status_t estado_conexion_wifi;
	bool b_conexion_wifi_timeout = 0;

	//WiFi.mode(WIFI_STA);
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
	modo_wifi_actual = WiFi.getMode();
	/*if(modo_wifi != modo_wifi_actual){
		//Si cambio el modo liberar todos los sockets.
		modo_wifi = modo_wifi_actual;
		//liberar_recursos();
	}*/
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

/**
 * Comando para obtener el RSSI del punto de acceso al cual se encuentra
 * conectado.
 *
 *  @param 	Ninguno.
 *  @retval 0,RSSI(dB) Sin error, se retorna el RSSI en decibeles.
 *  @retval 1 Error al obtener el RSSI.
 */
void cmd_WRI(){
	/*WRI - WiFi RSSI*/
	int32_t rssi = WiFi.RSSI();
	if(rssi == 31){
		/*Valor invalido, ver documentacion del SDK: wifi_station_get_rssi*/
		Serial.print('1');
	}else{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(rssi);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

/**
 * Comando para obtener el SSID de la estacion a la que se encuentra conectado
 * actualmente el modulo.
 *
 *  @param 	Ninguno.
 *  @retval 0,SSID\n	Sin error, se retorna el SSID.
 *  @retval 1 Error al obtener el SSID, no se encuentra conectada a ningun red.
 */

void cmd_WID(){
	int ssid_longitud;
	ssid_longitud = strlen(WiFi.SSID().c_str());
	if(ssid_longitud == 0){
		Serial.print('1');
	}else{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(WiFi.SSID().c_str());
	}
	Serial.print(CMD_TERMINATOR);
	return;
}


void cmd_WFI(){
	/*WFI - WiFi Station Information
	 * Descripcion: Imprime la direccion MAC e IP local de la  interfaz de estacion, ademas
	 * de la mascara de subred, IP de la puerta de enlance y la direccion IP del servidor DNS #1*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.macAddress());
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.localIP());
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.subnetMask());
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.gatewayIP().toString().c_str());
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.dnsIP());
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WFD(){
	/*WFD - WiFi Disconnect*/
	bool b_estacion_off = comando_recibido.parametros[0];
	if(!dentro_intervalo(b_estacion_off,0,1)){
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if( WiFi.disconnect(b_estacion_off) ){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print('2');
	}
	delay(100);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WAC(){
	/*WAC - WiFi Soft-AP Configuration*/
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

/**
 * Comando para configurar el modulo ESP8266 en modo punto de acceso(softAP).
 * @param SSID			Nombre del AP, 63 caracteres maximo.
 * @param Contraseña    Minimo 8 caracteres.
 * @param Canal         Numero del canal WiFi, del 1 al 13.
 * @param SSID_Oculto   0 para mostrar el SSID, 1 para ocultar.
 * @param MAX_CONEX    	Numero de conexiones simultaneas, del 1 al 4.
 * @retval 0			Sin error, el punto de acceso se creo correctamente.
 * @retval 1 			Error, el numero de canal esta fuera de rango.
 * @retval 2 			Error, la opcion de SSID_oculto esta fuera de rango.
 * @retval 3 			Error, la cantidad de conexiones esta fuera de rango.
 * @retval 4 			Error, no se pudo crear el punto de acceso.
 */

void cmd_WFA(){
	bool b_punto_acceso_creado = 0;
	uint8_t canal_wifi = 1;
	uint8_t hidden_opt = 0;
	uint8_t cant_max_clientes_wifi = 4;

	canal_wifi = atoi(comando_recibido.parametros[2]);
	hidden_opt = atoi(comando_recibido.parametros[3]);
	cant_max_clientes_wifi = atoi(comando_recibido.parametros[4]);

	if(!dentro_intervalo(canal_wifi,1,13)){
		/*El numero de canal_wifi esta fuera de rango*/
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(hidden_opt,0,1)){
		/*El numero de hidden_opt esta fuera de rango*/
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(cant_max_clientes_wifi,1,4)){
		/*El numero de cant_max_clientes_wifi esta fuera de rango*/
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
		return;
	}

	//WiFi.disconnect();
	delay(100);
	b_punto_acceso_creado = WiFi.softAP(comando_recibido.parametros[0], comando_recibido.parametros[1], canal_wifi, hidden_opt, cant_max_clientes_wifi);
	delay(5000);
	modo_wifi_actual = WiFi.getMode();
/*	if(modo_wifi != modo_wifi_actual){
		//Si cambio el modo liberar todos los sockets.
		modo_wifi = modo_wifi_actual;
		//liberar_recursos();
	}*/
	if(b_punto_acceso_creado == 0){
		Serial.print('4');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}



void cmd_WAS(){
	/*WAS - WiFi Acces Point Stations connected*/
	uint8_t estaciones_conectadas = 0;
	estaciones_conectadas = WiFi.softAPgetStationNum();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(estaciones_conectadas);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WAD(){
	/*WAD - WiFi Acces Point Disconnect*/
	bool b_softAP_off = comando_recibido.parametros[0];
	if(!dentro_intervalo(b_softAP_off,0,1)){
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if( WiFi.softAPdisconnect(b_softAP_off) ){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print('2');
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WAI(){
	/*WAI - WiFi Acces Point Information, IP and MAC*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.softAPIP());
	Serial.print(CMD_DELIMITER);
	Serial.print(WiFi.softAPmacAddress());
	Serial.print(CMD_TERMINATOR);
	return;
}

/**
 * CCS - Client Connect Server
 * Comando para conectarse a un servidor, sea TCP o UDP.
 * @param SSID			Nombre del AP, 63 caracteres maximo.
 * @param Contraseña    Minimo 8 caracteres.
 * @param Canal         Numero del canal WiFi, del 1 al 13.
 * @param SSID_Oculto   0 para mostrar el SSID, 1 para ocultar.
 * @param MAX_CONEX    	Numero de conexiones simultaneas, del 1 al 4.
 * @retval 0			Sin error, el punto de acceso se creo correctamente.
 * @retval 1 			Error, el numero de canal esta fuera de rango.
 * @retval 2 			Error, la opcion de SSID_oculto esta fuera de rango.
 * @retval 3 			Error, la cantidad de conexiones esta fuera de rango.
 * @retval 4 			Error, no se pudo crear el punto de acceso.
 */
/*Comandos para el manejo de las operaciones TCP*/
void cmd_CCS(){
	/*CCS - cliente_tcp Connect to Server*/
	char tipo_conexion[4];
	uint16_t puerto_conexion;
	uint8_t socket;
	int8_t conexion_wifi;
	int estado_conexion_al_servidor_tcp = 0;

	strcpy(tipo_conexion,comando_recibido.parametros[0]);
	puerto_conexion = atoi(comando_recibido.parametros[2]);

	if(!dentro_intervalo(puerto_conexion,0,NUM_MAX_PUERTO)){
		/*Puerto TCP fuera de rango*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}

	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
	}
	if(strcmp(tipo_conexion,"TCP") == 0)
	{
		socket = obtener_socket_libre();
		if(socket == 255){
			/*No hay socket disponible*/
			Serial.print('4');
			Serial.print(CMD_TERMINATOR);
			return;
		}
		estado_conexion_al_servidor_tcp = cliente_tcp[socket].connect(comando_recibido.parametros[1],puerto_conexion);
		if(estado_conexion_al_servidor_tcp){
			//cliente_tcp[socket].setNoDelay(1);
			socket_info[socket].tipo = TIPO_CLIENTE;
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket);
			Serial.print(CMD_TERMINATOR);
		}else{
			/*No se pudo conectar*/
			Serial.print('5');
			Serial.print(CMD_TERMINATOR);
		}
	}else if(strcmp(tipo_conexion,"UDP") == 0)
	{
		if( Udp.beginPacket(comando_recibido.parametros[1],puerto_conexion) )
		{
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_TERMINATOR);
		}else{
			Serial.print('6');
			Serial.print(CMD_TERMINATOR);
		}
	}else{
		/*Dar mensaje de error en el tipo de conexion*/
		Serial.print('7');
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

void cmd_SLC(){
	/*SLC - Server Listen to Clients*/
	wl_status_t estado_conexion_wifi;
	uint8_t socket_pasivo;
	uint8_t backlog;
	uint16_t puerto_tcp;
	int8_t conexion_wifi;
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
	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*Determinar si ya existe un servidor funcionando con ese puerto*/
	for (socket_pasivo = 0; socket_pasivo < CANT_MAX_SERVIDORES; socket_pasivo++) {
		if(puerto_tcp != servidor[socket_pasivo].num_puerto_en_uso){
			b_puerto_tcp_en_uso = false;
			break;
		}else{
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket_pasivo,DEC);
			Serial.print(CMD_TERMINATOR);
			return;
		}
	}
	/*Si no existe un servidor con ese puerto, determinar que servidor esta libre y crear el servidor*/
	if(!b_puerto_tcp_en_uso){
		for (socket_pasivo = 0; socket_pasivo < CANT_MAX_SERVIDORES; socket_pasivo++) {
			if(servidor_obj[socket_pasivo].status() == CLOSED){
				servidor[socket_pasivo].num_puerto_en_uso = puerto_tcp;
				servidor[socket_pasivo].cant_maxima_clientes_permitidos= backlog;
				servidor_obj[socket_pasivo].begin(puerto_tcp);
				Serial.print(CMD_RESP_OK);
				Serial.print(CMD_DELIMITER);
				Serial.print(socket_pasivo,DEC);
				Serial.print(CMD_TERMINATOR);
				servidor[socket_pasivo].b_activo = true;
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

//SDU-Send UDP, para enviar datos con el protocolo UDP
void cmd_SDU()
{
	uint8_t socket;
	uint16_t cant_bytes_enviar;
	socket = atoi(comando_recibido.parametros[0]);
	cant_bytes_enviar = atoi(comando_recibido.parametros[1]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES))
	{
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(cant_bytes_enviar,0,TAM_MAX_PAQUETE_DATOS_UDP))
	{
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if ( Udp.write(paquete_datos_tcp,cant_bytes_enviar) )
	{
		if(Udp.endPacket())
		{
			Serial.print('0');
			Serial.print(CMD_TERMINATOR);
		}else{
			Serial.print('4');
			Serial.print(CMD_TERMINATOR);
		}
	}else{
		Serial.print('3');
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

/*Comando RVU - Receive UDP Data.
 * */
void cmd_RVU()
{
	int cant_bytes_paquete_udp_recibido;
	uint8_t socket;
	int8_t conexion_wifi;
	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES)){
		Serial.print('1');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print('2');
		Serial.print(CMD_TERMINATOR);
		return;
	}
	cant_bytes_paquete_udp_recibido = Udp.parsePacket();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(cant_bytes_paquete_udp_recibido,DEC);
	Serial.print(CMD_DELIMITER);
	while(cant_bytes_paquete_udp_recibido--){
		Serial.print((char)Udp.read());
	}
	Serial.print(CMD_TERMINATOR);
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
