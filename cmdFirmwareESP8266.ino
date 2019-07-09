/**
 * TODO: - [X] Unificar los mensajes de errores que se repiten en los comandos. Por ej
 * que no este conectado a WiFi sea '1' para todos los comandos.
 *
 * */


#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

// https://github.com/arduino-libraries/NTPClient
#include "NTPClient.h"

//#include "test.h"
//#include "puerto_serial.h"
#include "operaciones_paquetes.h"
#include "cmd_definicion.h"

extern "C" {
  #include <user_interface.h>
}

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

std::vector<WiFiServer> servidor_obj(CANT_MAX_SERVIDORES,
		WiFiServer(NUM_PUERTO_SERVIDOR_DEFECTO));

std::vector<WiFiUDP> udp_obj(CANT_MAX_SERVIDORES,
		WiFiUDP());

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);


struct elementos_servidor{
	bool b_activo;
	uint16_t num_puerto_en_uso;
	uint8_t cant_maxima_clientes_permitidos;
	uint8_t cant_clientes_activos;
};
struct elementos_servidor servidor[CANT_MAX_SERVIDORES];

typedef enum
{ TIPO_SERVIDOR, TIPO_CLIENTE, NINGUNO } tipoSocket;

typedef enum
{ TCP, UDP, NONE} protocol;

struct sock_info
{
	bool en_uso;
	tipoSocket  tipo;
	protocol protocolo;
	int8_t indice_servidor;
	int8_t indice_objeto;
};

/* TODO: Verificar que al cambiar el modo WiFi con el comando WFM, que pasan con
 * las conexiones que estaban activas, si no es necesario actualizar la estructura.
 */
sock_info sockets[CANT_MAX_CLIENTES];

//#define sDebug 1
/*-------------------------------------------------------------------*/

bool dentro_intervalo(uint32_t val, uint32_t min, uint32_t max);


//----------------------------Prototipo de comandos-----------------------------
//Comandos basicos
void cmd_MIS(void);
void cmd_MRS(void);
void cmd_MVI(void);
void cmd_MDS(void);
void cmd_MUC(void);
void cmd_MRP(void);
void cmd_MFH(void);


//Comandos WiFI
void cmd_WFM(void);
void cmd_WFC(void);
void cmd_WFS(void);
void cmd_WFD(void);
void cmd_WFA(void);
void cmd_WSI(void);
void cmd_WCF(void);
void cmd_WAC(void);
void cmd_WMA(void);
void cmd_WSC(void);
void cmd_WSS(void);


void cmd_WSN(void);

//Comandos TCP/IP
void cmd_SOI(void);
void cmd_IDN(void);
void cmd_CCS(void);
void cmd_SOW(void);
void cmd_SOR(void);
void cmd_SOC(void);
void cmd_WFI(void);

void cmd_SLC(void);
void cmd_STC(void);
void cmd_STG(void);

//Comandos propios
void cmd_RVU(void);
void cmd_SVU(void);
void cmd_SDU(void);
void cmd_RVU(void);


void cmd_SCC(void);
void cmd_SAC(void);

void cmd_WRI(void);
void cmd_WID(void);

void cmd_WAS(void);
void cmd_WAD(void);

void cmd_WAI(void);

void cmd_WSD(void);

const struct cmd conjunto_comandos[CANT_MAX_CMD] =
{
		{"MIS",{0,0},&cmd_MIS},
		{"MRS",{0,0},&cmd_MRS},
		{"MVI",{0,0},&cmd_MVI},
		{"MDS",{2,0},&cmd_MDS},
		{"MUC",{1,0},&cmd_MUC},
		{"MRP",{1,0},&cmd_MRP},
		{"MFH",{0,0},&cmd_MFH},
		{"WFM",{1,0},&cmd_WFM},
  		{"WFC",{2,0},&cmd_WFC},
  		{"WFS",{0,0},&cmd_WFS},
		{"WFD",{1,0},&cmd_WFD},
		{"WFA",{5,0},&cmd_WFA},
		{"WSI",{0,0},&cmd_WSI},
		{"WCF",{4,0},&cmd_WCF},
		{"WAC",{3,0},&cmd_WAC},
		{"WMA",{2,0},&cmd_WMA},
		{"WSC",{0,0},&cmd_WSC},
		{"WSS",{0,0},&cmd_WSS},
		{"SOI",{1,0},&cmd_SOI},
		{"IDN",{1,0},&cmd_IDN},
		{"CCS",{2,3},&cmd_CCS},
		{"SOW",{2,0},&cmd_SOW},
		{"SOR",{1,0},&cmd_SOR},
		{"SOC",{1,0},&cmd_SOC},
		{"WFI",{0,0},&cmd_WFI},
		{"SLC",{2,0},&cmd_SLC},
		{"STC",{3,0},&cmd_STC},
		{"STG",{0,0},&cmd_STG},
		{"SVU",{1,0},&cmd_SVU},
		{"SDU",{2,0},&cmd_SDU},
		{"RVU",{1,0},&cmd_RVU},
		{"SCC",{1,0},&cmd_SCC},
		{"SAC",{1,0},&cmd_SAC},
		{"WRI",{0,0},&cmd_WRI},
		{"WID",{0,0},&cmd_WID},
		{"WSN",{1,0},&cmd_WSN},
  		{"WAS",{0,0},&cmd_WAS},
  		{"WAD",{1,0},&cmd_WAD},
		{"WSD",{0,0},&cmd_WSD},
  		{"WAI",{0,0},&cmd_WAI}
};

struct cmd_recibido comando_recibido;
char paquete_datos_tcp[TAM_MAX_PAQUETE_DATOS_TCP];
char paquete_serial[64];

size_t tam_buffer_serial = 0;

WiFiMode_t modo_wifi_actual;
wl_status_t estado_conexion_wifi_interfaz_sta_actual;

/**< Variable utilizada para indicar cuando se inicio la configuracion utilizando
 * el SmartConfig*/
bool b_smartconfig_en_proceso = false;

/**<Variable utilizada para indicar cuando se recibio el paquete de configuracion
 * de credeneciales para SmartConfig */
bool b_smartconfig_credenciales_recibidas = false;

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
int8_t	obtener_socket_libre(uint8_t protocolo)
{
	int8_t indice_socket_disponible;
	uint8_t indice_barrido;
	for (uint8_t indice_socket = 0; indice_socket < CANT_MAX_CLIENTES;
		++indice_socket)
	{
		if(sockets[indice_socket].en_uso == false)
		{
			indice_socket_disponible = indice_socket;
			break;
		}
		indice_socket_disponible = -1;
	}

	if(indice_socket_disponible == -1)
	{
		//No hay ningun socket disponible.
		return -1;
	}

	if(protocolo == TCP)
	{
		uint8_t i=0;
		for (i = 0; i < CANT_MAX_CLIENTES; i++)
		{
			/*Si esta vacio y no esta conectado*/
			if (!cliente_tcp[i] || !cliente_tcp[i].connected())
			{
				if (cliente_tcp[i])
				{
				  cliente_tcp[i].stop();
				}
				sockets[indice_socket_disponible].indice_objeto = i;
				return indice_socket_disponible;
			}
		}
		/*Si no hay ningun lugar disponible*/
		if (i == CANT_MAX_CLIENTES)
		{
			return -1;
		}
	}

	if(protocolo == UDP)
	{
		//Almacenar primero cuales objetos ya esta siendo utilizados
		for (uint8_t indice_socket = 0; indice_socket < CANT_MAX_CLIENTES;
			++indice_socket)
		{
			for (indice_barrido = 0; indice_barrido < CANT_MAX_CLIENTES;
					++indice_barrido)
			{
				if( (sockets[indice_barrido].en_uso == true) 	&&
					(sockets[indice_barrido].protocolo == UDP))
				{
					if(sockets[indice_barrido].indice_objeto == indice_socket)
					{
						break;
					}
				}
			}
			if(indice_barrido == CANT_MAX_CLIENTES)
			{
				sockets[indice_socket_disponible].indice_objeto = indice_socket;
				break;
			}
		}
		return indice_socket_disponible;
	}
	return -1;
}

/* Descripcion: Se encarga de aceptar a clientes que se quieren conectar al servidor*/
uint8_t servidor_acepta_clientes(WiFiServer& server, uint8_t serverId){
	uint8_t socket;
	/*Verificar que todavia no se conectaron el numero maximo de clientes*/
	if(servidor[serverId].cant_clientes_activos < servidor[serverId].cant_maxima_clientes_permitidos){
		socket = obtener_socket_libre(TCP);
		if(socket!=255){
			/*Se encontro socket libre*/
			/*Se acepta al cliente*/
			cliente_tcp[sockets[socket].indice_objeto] = server.available();
			servidor[serverId].cant_clientes_activos++;
			sockets[socket].tipo = TIPO_SERVIDOR;
			sockets[socket].indice_servidor = serverId;
			sockets[socket].en_uso = true;
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

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}

//----------------------------Comandos basicos----------------------------------

/**
 *  Comando que verifica que el modulo se encuentra encendido.
 *
 *  Hace un simple Serial.print para verificar que el modulo esta funcionando.
 *
 *  @param Ninguno.
 *  @return 0\n
 */
void cmd_MIS()
{
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
void cmd_MRS()
{
	/*MRS - Module Reset*/
	ESP.restart();
	return;
}

void cmd_MVI()
{
	//MVI - Module Version ID
	String arduino_core_version;
	arduino_core_version = ESP.getCoreVersion();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print("Firmware:V1.0.6");
	Serial.print(CMD_DELIMITER);
	Serial.print("ArduinoCore:");
	Serial.print(arduino_core_version);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_MDS()
{
	//MDS - Module deep sleep
	//Medido en ms.
	uint64_t tiempo_dormir_ms;
	uint8_t modo_rf;
	WakeMode modo;

	tiempo_dormir_ms = atoi(comando_recibido.parametros[0]);
	modo_rf = atoi(comando_recibido.parametros[1]);

	//Limitamos el maximo a 4294967295 ms (32 bits)
	if(!dentro_intervalo(tiempo_dormir_ms, 0, 0xFFFFFFFF))
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(!dentro_intervalo(modo_rf, 0, 3))
	{
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	switch (modo_rf)
	{
		case 0:
			modo = RF_DEFAULT;
			break;
		case 1:
			modo = RF_CAL;
			break;
		case 2:
			modo = RF_NO_CAL;
			break;
		case 3:
			modo = RF_DISABLED;
			break;
		default:
			break;
	}

	ESP.deepSleep(tiempo_dormir_ms, modo);

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
void cmd_MUC()
{
	/*Module UART Configuration*/
	uint32_t baud_rate = atoi(comando_recibido.parametros[0]);
	//Serial.print(baud_rate);
	//Serial.print('\n');
	if(!dentro_intervalo(baud_rate,9600,MAX_BAUD_RATE)){
		/*Baud rate fuera de rango*/
		Serial.print(CMD_ERROR_1);
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

void cmd_MRP()
{
	//MRP - Module RF Power
	float potencia_dbm;
	char *prt;

	potencia_dbm = strtod(comando_recibido.parametros[0], &prt);

	if( (potencia_dbm > 20.5) || (potencia_dbm < 0)  )
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	WiFi.setOutputPower(potencia_dbm);
	delay(1);

	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);

	return;
}

/**
 *  Comando que imprime la cantidad de RAM disponible en Bytes.
 *
 *  @param 	Ninguno.
 *  @return Bytes disponibles de RAM en el modulo.
 */
void cmd_MFH()
{
	/*GFH - Get Free Heap*/
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(ESP.getFreeHeap(),DEC);
	Serial.print(CMD_TERMINATOR);
	return;
}

//----------------------------Comandos WiFi-------------------------------------

void cmd_WFM()
{
	uint8_t parametro_modo_wifi;
	WiFiMode_t modo_wifi;
	bool ret_val;
	parametro_modo_wifi = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(parametro_modo_wifi,0,3) == true){
		Serial.print(CMD_ERROR_1);
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
	if(ret_val == true)
	{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_TERMINATOR);
	}else{
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
	}
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
void cmd_WFC()
{
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
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	switch(estado_conexion_wifi){
		case WL_IDLE_STATUS:
			Serial.print(CMD_ERROR_7);
			break;
		case WL_NO_SSID_AVAIL:
			Serial.print(CMD_ERROR_4);
			break;
		case WL_SCAN_COMPLETED:
			Serial.print(CMD_ERROR_5);
			break;
		case WL_CONNECT_FAILED:
			/*Significa que la contraseña es incorrecta*/
			Serial.print(CMD_ERROR_3);
			break;
		case WL_CONNECTION_LOST:
			Serial.print(CMD_ERROR_6);
			break;
		case WL_DISCONNECTED:
			Serial.print(CMD_ERROR_1);
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
 *  Comando que explora los puntos de acceso dentro del alcance del modulo.
 *
 *  @param 	Ninguno.
 *  @return 0,NRO_APS_DISPONIBLES,SSID1;RSSI1,SSID2;RSSI2,SSIDn;RSSIn\n
 */
void cmd_WFS()
{
	/*WFS - WiFi Scan*/
	ESP8266WiFiScanClass WiFiScan;
	int cant_punto_acceso_encontrados;
	cant_punto_acceso_encontrados = WiFiScan.scanNetworks();
	if (cant_punto_acceso_encontrados == -1) {
		Serial.print(CMD_ERROR_1);
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

void cmd_WFD()
{
	/*WFD - WiFi Disconnect*/
	bool b_estacion_off = comando_recibido.parametros[0];
	if(!dentro_intervalo(b_estacion_off,0,1)){
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if( WiFi.disconnect(b_estacion_off) ){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print(CMD_ERROR_2);
	}
	delay(100);
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
void cmd_WFA()
{
	bool b_punto_acceso_creado = 0;
	uint8_t canal_wifi = 1;
	uint8_t hidden_opt = 0;
	uint8_t cant_max_clientes_wifi = 4;

	canal_wifi = atoi(comando_recibido.parametros[2]);
	hidden_opt = atoi(comando_recibido.parametros[3]);
	cant_max_clientes_wifi = atoi(comando_recibido.parametros[4]);

	if(!dentro_intervalo(canal_wifi,1,13)){
		/*El numero de canal_wifi esta fuera de rango*/
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(hidden_opt,0,1)){
		/*El numero de hidden_opt esta fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(cant_max_clientes_wifi,1,4)){
		/*El numero de cant_max_clientes_wifi esta fuera de rango*/
		Serial.print(CMD_ERROR_3);
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
		Serial.print(CMD_ERROR_4);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WSI()
{
	//WSI - WiFi SoftAP Information
	uint8_t cantidad_clientes_conectados;
	struct station_info *info_cliente;
	struct ip_addr *direccion_ip;
	IPAddress direccion;

	cantidad_clientes_conectados = WiFi.softAPgetStationNum();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(cantidad_clientes_conectados);

	for (int cliente = 0; cliente < cantidad_clientes_conectados; cliente++)
	{
		Serial.print(CMD_DELIMITER);
		info_cliente = wifi_softap_get_station_info();
		direccion_ip = &info_cliente->ip;
		direccion = direccion_ip->addr;

		Serial.print(direccion);

		Serial.print(";");

		Serial.print(info_cliente->bssid[0],HEX);Serial.print(":");
		Serial.print(info_cliente->bssid[1],HEX);Serial.print(":");
		Serial.print(info_cliente->bssid[2],HEX);Serial.print(":");
		Serial.print(info_cliente->bssid[3],HEX);Serial.print(":");
		Serial.print(info_cliente->bssid[4],HEX);Serial.print(":");
		Serial.print(info_cliente->bssid[5],HEX);
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
void cmd_WCF()
{
	IPAddress ip,dns,gateway,subnet;
	if(!ip.fromString(comando_recibido.parametros[0])){
		/*IP Invalida*/
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dns.fromString(comando_recibido.parametros[1])){
		/*DNS Invalido*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(comando_recibido.parametros[2])){
		/*Gateway Invalido*/
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(comando_recibido.parametros[3])){
		/*Subnet Invalido*/
		Serial.print(CMD_ERROR_4);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(WiFi.config(ip,gateway,subnet,dns)){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print(CMD_ERROR_5);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WAC()
{
	/*WAC - WiFi Soft-AP Configuration*/
	bool b_configuracion_punto_acceso = 0;
	IPAddress ip_local;
	IPAddress gateway;
	IPAddress subnet;
	if(!ip_local.fromString(comando_recibido.parametros[0])){
		/*IP local Invalido*/
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(comando_recibido.parametros[1])){
		/*Gateway Invalido*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(comando_recibido.parametros[2])){
		/*Subnet Invalido*/
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	b_configuracion_punto_acceso = WiFi.softAPConfig(ip_local, gateway, subnet);
	if(b_configuracion_punto_acceso == 0){
		Serial.print(CMD_ERROR_4);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;

}

void cmd_WMA()
{
	/*WMA - WiFi Mac Address*/
	uint8_t parametro_interfaz;
	uint8_t mac[6];
	uint8_t cant_bytes_mac;

	parametro_interfaz = atoi(comando_recibido.parametros[0]);

	if(!dentro_intervalo(parametro_interfaz, 0, 1))
	{
		//Parametro de interfaz fuera de rango
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	cant_bytes_mac = strlen(comando_recibido.parametros[1]);

	if(cant_bytes_mac != 17)
	{
		//Direccion de MAC con longitud incorrecta, muy larga o muy corta.
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	parseBytes(comando_recibido.parametros[1], ':', mac, 6, 16);

	//Parametro_interfaz = 0, para interfaz de estacion (STA)
	//Parametro_interfaz = 1, para interfaz de punto de acceso (softAP)
	if(wifi_set_macaddr(parametro_interfaz,mac))
	{
		Serial.print(CMD_RESP_OK);
	}
	else
	{
		Serial.print(CMD_ERROR_3);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

/*Comando para iniciar la configuracion utilizando SmartConfig*/
void cmd_WSC()
{
	if(WiFi.beginSmartConfig())
	{
		b_smartconfig_en_proceso = true;
		Serial.print(CMD_RESP_OK);
	}else
	{
		Serial.print(CMD_ERROR_1);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

/*Comando para detener la configuracion a traves de SmartConfig*/
void cmd_WSS()
{
	if(WiFi.stopSmartConfig())
	{
		b_smartconfig_en_proceso = false;
		b_smartconfig_credenciales_recibidas = false;
		Serial.print(CMD_RESP_OK);
	}else
	{
		b_smartconfig_credenciales_recibidas = false;
		Serial.print(CMD_ERROR_1);
	}
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
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if( WiFi.softAPdisconnect(b_softAP_off) ){
		Serial.print(CMD_RESP_OK);
	}else{
		Serial.print(CMD_ERROR_2);
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

/*Comando para verificar el estado de la configuracion SmartConfig*/
void cmd_WSD()
{
	if(b_smartconfig_credenciales_recibidas == true)
	{
		b_smartconfig_credenciales_recibidas = false;
		Serial.print(CMD_RESP_OK);
	}else
	{
		Serial.print(CMD_ERROR_1);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

//----------------------------Comandos TCP/IP-----------------------------------
void cmd_SOI()
{
	uint8_t socket;

	socket = atoi(comando_recibido.parametros[0]);

	if(!dentro_intervalo(socket, 0, CANT_MAX_CLIENTES))
	{
		//Socket fuera de rango
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);

	//Tipo de conexion: TCP o UDP
	if(sockets[socket].protocolo == TCP)
	{
		Serial.print("TCP");
		Serial.print(CMD_DELIMITER);
		//IP remota
		Serial.print(cliente_tcp[sockets[socket].indice_objeto].remoteIP().toString());
		Serial.print(CMD_DELIMITER);
		//Puerto remoto
		Serial.print(cliente_tcp[sockets[socket].indice_objeto].remotePort());
		Serial.print(CMD_DELIMITER);
		//Puerto local
		Serial.print(cliente_tcp[sockets[socket].indice_objeto].localPort());
		Serial.print(CMD_DELIMITER);
	}
	else if(sockets[socket].protocolo == UDP)
	{
		Serial.print("UDP");
		Serial.print(CMD_DELIMITER);
		Serial.print(udp_obj[sockets[socket].indice_objeto].remoteIP().toString());
		Serial.print(CMD_DELIMITER);
		Serial.print(udp_obj[sockets[socket].indice_objeto].remotePort());
		Serial.print(CMD_DELIMITER);
		Serial.print(udp_obj[sockets[socket].indice_objeto].localPort());
		Serial.print(CMD_DELIMITER);
	}

	//El socket es un cliente o servidor
	switch (sockets[socket].tipo)
	{
		case TIPO_CLIENTE:
			Serial.print('1');
			break;
		case TIPO_SERVIDOR:
			Serial.print('0');
			break;
		default:
			break;
	}

	Serial.print(CMD_TERMINATOR);

	return;
}

void cmd_IDN()
{
	//IDN - IP Domain Name
	IPAddress direccion_ip;
	char nombre_servidor[32];

	strcpy(nombre_servidor,comando_recibido.parametros[0]);

	if ( WiFi.hostByName(nombre_servidor, direccion_ip) )
	{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(direccion_ip.toString());
	}
	else
	{
		//No se pudo resolver el nombre del servidor
		Serial.print(CMD_ERROR_1);
	}
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
void cmd_CCS()
{
	/*TODO: Se podria agregar verificacion para el parametro IP. Es decir
	 * comprobar que sea una direccion IP y que se tenga alcance a esta direccion.*/
	/*CCS - cliente_tcp Connect to Server*/
	char tipo_conexion[4];
	uint16_t puerto_conexion;
	int8_t socket;
	int8_t conexion_wifi;
	int estado_conexion_al_servidor_tcp = 0;

	strcpy(tipo_conexion,comando_recibido.parametros[0]);
	puerto_conexion = atoi(comando_recibido.parametros[2]);

	if(!dentro_intervalo(puerto_conexion,0,NUM_MAX_PUERTO)){
		/*Puerto TCP fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
	}
	if(strcmp(tipo_conexion,"TCP") == 0)
	{
		socket = obtener_socket_libre(TCP);
		if(socket == -1)
		{
			/*No hay socket disponible*/
			Serial.print(CMD_ERROR_3);
			Serial.print(CMD_TERMINATOR);
			return;
		}
		estado_conexion_al_servidor_tcp = cliente_tcp[sockets[socket].indice_objeto].connect(comando_recibido.parametros[1],puerto_conexion);
		if(estado_conexion_al_servidor_tcp){
			//cliente_tcp[socket].setNoDelay(1);
			sockets[socket].en_uso = true;
			sockets[socket].tipo = TIPO_CLIENTE;
			sockets[socket].protocolo = TCP;
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket);
			Serial.print(CMD_TERMINATOR);
		}else{
			/*No se pudo conectar*/
			Serial.print(CMD_ERROR_4);
			Serial.print(CMD_TERMINATOR);
		}
	}else if(strcmp(tipo_conexion,"UDP") == 0)
	{
		socket = obtener_socket_libre(UDP);
		if(socket == -1)
		{
			/*No hay socket disponible*/
			Serial.print(CMD_ERROR_3);
			Serial.print(CMD_TERMINATOR);
			return;
		}
		if( udp_obj[sockets[socket].indice_objeto].beginPacket(comando_recibido.parametros[1],puerto_conexion) )
		{
			sockets[socket].en_uso = true;
			sockets[socket].tipo = TIPO_CLIENTE;
			sockets[socket].protocolo = UDP;
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_DELIMITER);
			Serial.print(socket);
			Serial.print(CMD_TERMINATOR);
		}else{
			Serial.print(CMD_ERROR_4);
			Serial.print(CMD_TERMINATOR);
		}
	}else{
		/*Dar mensaje de error en el tipo de conexion*/
		Serial.print(CMD_ERROR_5);
		Serial.print(CMD_TERMINATOR);
	}
	//Serial.print("Objeto Numero: ");
	//Serial.println(sockets[socket].indice_objeto,DEC);
	return;
}

/*SOW - Socket Write*/
void cmd_SOW()
{
	uint8_t socket = 255;
	int cant_bytes_enviar_tcp = -1;
	int cant_bytes_enviados_tcp = -2;
	int8_t conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	cant_bytes_enviar_tcp = atoi(comando_recibido.parametros[1]);
	conexion_wifi = verificar_conexion_wifi();

	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES))
	{
		/*Numero de socket fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(sockets[socket].protocolo == UDP)
	{
		//Socket del tipo incorrecto
		Serial.print(CMD_ERROR_4);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(!dentro_intervalo(cant_bytes_enviar_tcp,0,TAM_MAX_PAQUETE_DATOS_TCP))
	{
		/*Numero de bytes para escribir fuera de rango*/
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(!cliente_tcp[sockets[socket].indice_objeto].connected())
	{
		/*El socket no esta conectado*/
		sockets[socket].en_uso = false;
		Serial.print(CMD_ERROR_5);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	cant_bytes_enviados_tcp = cliente_tcp[sockets[socket].indice_objeto].write(paquete_datos_tcp,
			cant_bytes_enviar_tcp);
	if(cant_bytes_enviar_tcp != cant_bytes_enviados_tcp)
	{
		/*No se pudo escribir los datos al socket*/
		Serial.print(CMD_ERROR_6);
		Serial.print(CMD_TERMINATOR);
	}
	else
	{
		/*Los datos se enviaron correctamente*/
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_TERMINATOR);
	}
	return;
}

/*SOR - Socket Read*/
void cmd_SOR()
{
	uint8_t socket;
	int8_t conexion_wifi;
	uint16_t bytes_disponible_para_recibir = 0;

	socket = atoi(comando_recibido.parametros[0]);
	/*print received data from server*/
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES)){
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(sockets[socket].protocolo == UDP)
	{
		//Socket del tipo incorrecto
		Serial.print(CMD_ERROR_4);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
	}

	/*Agregar verificacion de conexion a servidor antes de imprimir respuesta*/
	if(!cliente_tcp[sockets[socket].indice_objeto].connected())
	{
		sockets[socket].en_uso = false;
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	bytes_disponible_para_recibir = cliente_tcp[sockets[socket].indice_objeto].available();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(bytes_disponible_para_recibir,DEC);
	Serial.print(CMD_DELIMITER);
	while(bytes_disponible_para_recibir--){
		Serial.print((char)cliente_tcp[sockets[socket].indice_objeto].read());
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SOC()
{
	/*SOC- Socket Close*/
	uint8_t socket;
	int8_t conexion_wifi;
	uint8_t server_id;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES) == true){
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	conexion_wifi = verificar_conexion_wifi();

	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
	}

	if(sockets[socket].en_uso == false)
	{
		//El socket no esta siendo utilizado.
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
	}

	if(sockets[socket].protocolo == TCP)
	{
		cliente_tcp[sockets[socket].indice_objeto].stop();
	}
	else if(sockets[socket].protocolo == UDP)
	{
		udp_obj[sockets[socket].indice_objeto].stop();
	}

	if(sockets[socket].tipo == TIPO_SERVIDOR)
	{
		server_id = sockets[socket].indice_servidor;
		servidor[server_id].cant_clientes_activos--;
		sockets[socket].indice_servidor = -1;
	}
	sockets[socket].tipo = NINGUNO;
	sockets[socket].en_uso = false;
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_WFI()
{
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

void cmd_SLC()
{
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
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dentro_intervalo(backlog,0,CANT_MAX_CLIENTES)){
		/*El numero de clientes esta fuera de rango*/
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}else{
		/*Verificar que se tienen los recursos disponibles para escuchar la cantidad de clientes*/
	}
	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
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

void cmd_STC()
{
	//STC - SNTP server configuration
	char nombre_servidor_pool[32];
	char *ptr;
	int offset_tiempo;
	unsigned long intervalo_refresco;

	strcpy(nombre_servidor_pool, comando_recibido.parametros[0]);
	offset_tiempo = atoi(comando_recibido.parametros[1]);
	intervalo_refresco = strtoul(comando_recibido.parametros[2], &ptr, 10);


	timeClient.setPoolServerName(nombre_servidor_pool);
	timeClient.setTimeOffset(offset_tiempo);
	timeClient.setUpdateInterval(intervalo_refresco);

	timeClient.begin();


	return;
}

void cmd_STG()
{
	//STG - STNP time get
	String tiempo;

	if ( !timeClient.update() )
	{
		//Error al actualizar el tiempo
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	tiempo = timeClient.getFormattedTime();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
	Serial.print(tiempo);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SAC()
{
	/*SAC - Server Accept Clients*/

	uint8_t socket;
	int8_t conexion_wifi;
	uint8_t serverAcceptStatus;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_SERVIDORES)){
		/*El numero de socket esta fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(servidor[socket].b_activo){
		if(servidor_obj[socket].hasClient()){
			serverAcceptStatus = servidor_acepta_clientes(servidor_obj[socket],socket);
			if(serverAcceptStatus == 1){
				/*Hay socket disponible*/
			}
			if(serverAcceptStatus == 2){
				/*No hay socket disponible*/
				Serial.print(CMD_ERROR_3);
			}
			if(serverAcceptStatus == 3){
				/*No se aceptan mas clientes en este servidor*/
				Serial.print(CMD_ERROR_6);
			}
		}else{
			/*El servidor no tiene clientes*/
			Serial.print(CMD_ERROR_4);
		}
	}else{
		/*El servidor se encuentra desactivado*/
		Serial.print(CMD_ERROR_5);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SCC()
{
	/*SCC - Server Close Connection*/
	uint8_t socket;
	int8_t conexion_wifi;

	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_SERVIDORES)){
		/*El numero de socket esta fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	servidor_obj[socket].stop();
	servidor[socket].b_activo = false;
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_TERMINATOR);
	return;
}

void cmd_SVU()
{
	int8_t socket;
	uint16_t puerto_udp;
	int8_t conexion_wifi;

	puerto_udp = atoi(comando_recibido.parametros[0]);

	/*Determinar primero si el puerto es valido*/
	if(!dentro_intervalo(puerto_udp,0,NUM_MAX_PUERTO)){
		/*El numero de puerto esta fuera de rango*/
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	socket = obtener_socket_libre(UDP);
	if(socket == -1)
	{
		/*No hay socket disponible*/
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if( udp_obj[sockets[socket].indice_objeto].begin(puerto_udp))
	{
		sockets[socket].en_uso = true;
		sockets[socket].tipo = TIPO_SERVIDOR;
		sockets[socket].protocolo = UDP;
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(socket);
	}else{
		Serial.print(CMD_ERROR_4);
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

//SDU-Send UDP, para enviar datos con el protocolo UDP
void cmd_SDU()
{
	uint8_t socket;
	uint16_t cant_bytes_enviar;
	int8_t conexion_wifi;
	socket = atoi(comando_recibido.parametros[0]);
	cant_bytes_enviar = atoi(comando_recibido.parametros[1]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES))
	{
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	if(sockets[socket].protocolo == TCP)
	{
		//Socket del tipo incorrecto
		Serial.print(CMD_ERROR_5);
		Serial.print(CMD_TERMINATOR);
		return;
	}

	//TODO: Verificar que el socket sea uno del tipo cliente y no servidor.

	if(!dentro_intervalo(cant_bytes_enviar,0,TAM_MAX_PAQUETE_DATOS_UDP))
	{
		Serial.print(CMD_ERROR_3);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if ( udp_obj[sockets[socket].indice_objeto].write(paquete_datos_tcp,cant_bytes_enviar) )
	{
		if(udp_obj[sockets[socket].indice_objeto].endPacket())
		{
			Serial.print(CMD_RESP_OK);
			Serial.print(CMD_TERMINATOR);
		}else{
			Serial.print(CMD_ERROR_4);
			Serial.print(CMD_TERMINATOR);
		}
	}else{
		Serial.print(CMD_ERROR_4);
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
	int8_t indice_objeto;
	int8_t conexion_wifi;
	socket = atoi(comando_recibido.parametros[0]);
	if(!dentro_intervalo(socket,0,CANT_MAX_CLIENTES)){
		Serial.print(CMD_ERROR_2);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	/*Verificar conexion WiFi*/
	conexion_wifi = verificar_conexion_wifi();
	if(conexion_wifi != 0)
	{
		Serial.print(CMD_ERROR_1);
		Serial.print(CMD_TERMINATOR);
		return;
	}
	indice_objeto = sockets[socket].indice_objeto;
	cant_bytes_paquete_udp_recibido = udp_obj[indice_objeto].parsePacket();
	Serial.print(CMD_RESP_OK);
	Serial.print(CMD_DELIMITER);
/*	Serial.print(udp_obj[indice_objeto].remoteIP());
	Serial.print(CMD_DELIMITER);
	Serial.print(udp_obj[indice_objeto].remotePort());
	Serial.print(CMD_DELIMITER);*/
	Serial.print(cant_bytes_paquete_udp_recibido,DEC);
	Serial.print(CMD_DELIMITER);
	while(cant_bytes_paquete_udp_recibido--){
		Serial.print((char)udp_obj[indice_objeto].read());
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

/* Comando para cambiar el hostname del servidor DHCP de la interfaz de estacion*/
void cmd_WSN()
{
	if (WiFi.hostname(comando_recibido.parametros[0]))
	{
		Serial.print(CMD_RESP_OK);

	}else
	{
		Serial.print(CMD_ERROR_1);
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
void cmd_WRI()
{
	/*WRI - WiFi RSSI*/
	int32_t rssi = WiFi.RSSI();
	if(rssi == 31){
		/*Valor invalido, ver documentacion del SDK: wifi_station_get_rssi*/
		Serial.print(CMD_ERROR_1);
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

void cmd_WID()
{
	int ssid_longitud;
	ssid_longitud = strlen(WiFi.SSID().c_str());
	if(ssid_longitud == 0){
		Serial.print(CMD_ERROR_1);
	}else{
		Serial.print(CMD_RESP_OK);
		Serial.print(CMD_DELIMITER);
		Serial.print(WiFi.SSID().c_str());
		Serial.print(CMD_DELIMITER);
		Serial.print(WiFi.psk().c_str());
	}
	Serial.print(CMD_TERMINATOR);
	return;
}

void setup()
{
	Serial.begin(115200);
	Serial.setRxBufferSize(1024);
	delay(10);

#ifdef sDebug
	Serial.println();
    Serial.println();
    Serial.println("Listo.");
    Serial.println("Las instrucciones que tengo son: ");
    for(int i=0; i < CANT_MAX_CMD; i++){
  	  Serial.println((instructionSet[i]));
    }
#endif

    for (uint8_t indice_socket = 0; indice_socket < CANT_MAX_CLIENTES; ++indice_socket) {
    	sockets[indice_socket].en_uso = false;
		sockets[indice_socket].tipo = NINGUNO;
		sockets[indice_socket].protocolo = NONE;
		sockets[indice_socket].indice_servidor = -1;
	}

    /*cliente_tcp[0].setNoDelay(1);
    cliente_tcp[1].setNoDelay(1);
    cliente_tcp[2].setNoDelay(1);
    cliente_tcp[3].setNoDelay(1);*/
    modo_wifi_actual = WiFi.getMode();
    estado_conexion_wifi_interfaz_sta_actual = WiFi.status();

    /*Inciar el sistema con la radio WiFi apagada*/
    WiFi.disconnect(1);
    delay(100);
    WiFi.mode(WIFI_OFF);

    Serial.print("R");
    Serial.print(CMD_TERMINATOR);
}

void loop()
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
	if (recibir_paquetes(paquete_serial, paquete_datos_tcp) == 1)
	{
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

		if(indice_comando != -1)
		{
			/*Se verifica que se recibio la cantidad necesaria de parametros para ejectuar el comando.*/
			if( validar_cantidad_parametros(indice_comando, comando_recibido.cantidad_parametros_recibidos))
			{

				/*Se llama a la funcion que ejecutara las acciones correspondientes al comando.*/
				conjunto_comandos[indice_comando].ejecutar();

			}
			else
			{
				/*No se cuenta con la cantidad de parametros necesarios*/
				Serial.print(CMD_NO_PARAM);
				Serial.print(CMD_TERMINATOR);
			}
		}
		else
		{
			/*No se encontro el comando*/
			Serial.print(CMD_NOT_FOUND);
			Serial.print(CMD_TERMINATOR);
		}
		memset(paquete_serial,0,sizeof(paquete_serial));
		memset(paquete_datos_tcp,0,sizeof(paquete_datos_tcp));
	}
	else
	{
		/*Problemas en la recepcion de paquete serial*/
	}

	yield();

	servidor_verificar_backlog();

	if(b_smartconfig_en_proceso == true)
	{
		if(WiFi.smartConfigDone())
		{
			b_smartconfig_credenciales_recibidas = true;
			b_smartconfig_en_proceso = false;
		}
	}
}
