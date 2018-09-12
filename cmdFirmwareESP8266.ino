#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>


#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>

/*Considerar usar client.setNoDelay para desactivar el algoritmo de naggle*/

/*-------------------------DEFINE's--------------------------------*/
//#define sDebug

/*---	Comandos	---*/

/*Cantidad maxima de parametros por comando*/
#define MAX_PARAMETERS 4

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define MAX_CHARS_PARAMETERS 1024

/*Cantidad maxima de caracteres que puede contener el comando*/
#define MAX_CHAR_INST 3

/*Numero maximo de instrucciones*/
#define MAX_INTS_SET 16

/*---------------------*/
/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define MAX_SERVER_TO 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define MAX_WIFICONNECT_TO 20000

/*Maximum number of Bytes for a packet*/
#define MAX_PACKET_SIZE 512

/*Numero maximo de clientes que puede manejar el modulo*/
#define MAX_NUM_CLIENTS 4

/*Numero maximo de servidores que puede manejar el modulo*/
#define MAX_NUM_SERVERS 4

/*Numero maximo de puerto*/
#define MAX_PORT_NUMBER  65535

/*Puerto por default que el server escuchara*/
#define DEFAULT_SERVER_PORT 80
/*-------------------------------------------------------------------*/

/*Array de parametros e instruccion*/
char bufferSerial[MAX_PARAMETERS+1][MAX_CHARS_PARAMETERS+1] = {{'\0'}};

/*Delimitador de la instruccion*/
char delimiter[2] = ",";

/*Cantidad de delimitadores que se encontro*/
uint8_t delimFound = 0;

/*Cantidad de parametros que se encontro*/
uint8_t parametersFound = 0;

/*Caracter que termina la instruccion*/
char endMarker = '\n';

//Campo de instruccion, +1 para el NULL al final
char INST[MAX_CHAR_INST+1] = {'\0'};

/*Array donde se almacenan los parametros*/
char parametros[MAX_PARAMETERS][MAX_CHARS_PARAMETERS+1] = {{'\0'}};

/*Indice que indica cual funcion se recibio*/
uint8_t	instructionIndex = 255;

/*Paquete para enviar a traves de la red*/
char packet[MAX_PACKET_SIZE+1];

/*Define el tamaño del buffer serial, +qParameters es para las comas que vienen con el comando.*/
const uint16_t numChars = MAX_CHAR_INST + MAX_CHARS_PARAMETERS + MAX_PACKET_SIZE + MAX_PARAMETERS;

/*Declaracion de los buffers seriales*/
char tempChars[numChars+1];
char serialCharsBuffer[numChars+1]; // an array to store the received data

/*Declaracion de los buffers utilizados para recibir datos del servidor*/
char	bufferReceivedFromServer[MAX_NUM_CLIENTS][MAX_PACKET_SIZE+1];
uint16_t bytesReceivedFromServer[MAX_NUM_CLIENTS];
bool	fullBufferRcvd[MAX_NUM_CLIENTS];

/*Bandera para indicar que el servidor esta activo*/
bool	serverOn[MAX_NUM_SERVERS] = {false,false,false,false};

/*Bandera utilizada para notificar que hay datos seriales nuevos*/
boolean newData = false;

/*Matriz que almacena los nombres de todas los comandos validos
* dejar con static? Podria afectar la velocidad*/
static const char instructionSet[MAX_INTS_SET][MAX_CHAR_INST+1] = {"WFC",	//0
		"WFS",	//1
		"WRI",	//2
		"WID",	//3
		"WFD",	//4
		"WCF",	//5
		"CCS",	//6
		"SOW",	//7
		"SOR",	//8
		"SOC",	//9
		"SLC",	//10
		"SCC",	//11
		"SAC",	//12
		"SRC",	//13
		"GFH",	//14
		"MIS"};	//15

/*Matriz que almacena la cantidad de parametros necesarios
 *por cada comando, correspondencia por indice.*/
const uint8_t qParametersInstruction[MAX_INTS_SET] ={2,0,0,0,0,4,2,3,1,1,1,1,1,1,0,0};

/*Declaracion del objeto que se utilizara para el manejo del cliente, maximo 4
 * por limitacion del modulo.*/
WiFiClient client[MAX_NUM_CLIENTS];

/*Declaracion de los objetos que se utilizaran para el manejo del servidor*/
std::vector<WiFiServer> server(MAX_NUM_SERVERS, WiFiServer(DEFAULT_SERVER_PORT));

uint8_t socketInUse[MAX_NUM_CLIENTS] = {0,0,0,0};
uint16_t serverPortsInUse[MAX_NUM_SERVERS];

//#define sDebug 1
/*-------------------------------------------------------------------*/


void setup() {
	Serial.begin(115200);
	delay(10);
	Serial.println();
	for(int i=0; i < MAX_NUM_CLIENTS; i++){
		bytesReceivedFromServer[i] = 0;
		fullBufferRcvd[i] = false;
	}

#ifdef sDebug
	Serial.println();
    Serial.println();
    Serial.println("Listo.");
    Serial.println("Las instrucciones que tengo son: ");
    for(int i=0; i < MAX_INTS_SET; i++){
  	  Serial.println((instructionSet[i]));
    }
#endif

    Serial.println("R");

}

void loop() {
	#ifdef sDebug
	unsigned long pMillis;
	unsigned long cMillis;
	unsigned long loopTime;
	#endif

	/*Recibe los datos por serial, hasta que se encuentra el caracter terminador.*/
	recvWithEndMarker();

	/*Una vez que se reciben todos los datos por serial, se conienza a procesar*/
	if(newData == true){

	#ifdef sDebug
		pMillis = millis();
		/*Echo*/
		showNewData();
	#endif

		/*Se copian los datos recibidos en un array temporal para su manipulacion.*/
		strcpy(tempChars, serialCharsBuffer);

		/*Separar los datos en los campos correspondientes.*/
		parseData();

	#ifdef sDebug
		/*Se muestran los datos separados en campos.*/

	#endif
		//showParsedData();

		yield();

		if(searchInstruction() == true){
			if(validateParameters() == true){
				runInstruction();
			}else{
				Serial.println("NEP");
			}
		}else{
			Serial.println("NOCMD");
		}

		yield();

		/*Se limpian todas las variables para recibir el siguiente comando.*/
		memset(tempChars, 0, sizeof(tempChars));
		memset(serialCharsBuffer, 0, sizeof(serialCharsBuffer));
		memset(bufferSerial, 0, sizeof(bufferSerial));
		memset(INST, 0, sizeof(INST));
		memset(parametros, 0, sizeof(parametros));
		delimFound = 0;
		parametersFound = 0;
		newData = false;



	#ifdef sDebug
		cMillis = millis();
		loopTime = cMillis - pMillis;
		Serial.print("Loop running time: ");
		Serial.print(loopTime);
		Serial.println("ms");
	#endif
	}
	yield();

	/*Se almacenan los datos recibidos en los sockets en sus respectivos buffer's*/
	receiveFromServer();

	yield();

}

/*
 * Descripcion: Compara la cadena que contiene el campo de instruccion(INST) con el array
 * de instrucciones(instructionSet). Si encuentra una coincidencia, guarda el indice y la
 * funcion retorna 1.
 * */
bool searchInstruction(){
	uint8_t j;
	for(j=0; j<MAX_INTS_SET; j++){
		if(strcmp(INST,instructionSet[j]) == 0){
			instructionIndex = j;
			return 1;

		}
	}
	return 0;
}

/* Descripcion: Comprueba que se hayan recibido la cantidad necesaria de parametros ejecutar el comando.*/
bool validateParameters(){
	if(qParametersInstruction[instructionIndex] == parametersFound){
		return 1;
	}
	return 0;
}

/* Descripcion: Ejectua el comando solicitado.*/
void runInstruction(){
	unsigned long previousMillis;
	unsigned long currentMillis;
	uint16_t port;
	int bytesToWrite, bytesWritten;
	int numSsid;
	uint8_t socket;
	uint8_t i;
	bool WFC_STATUS = 1, portInUse = false;
	wl_status_t WF_STATUS;
	int C_STATUS = 0;
	IPAddress ip,dns,gateway,subnet;

	switch(instructionIndex){
	case 0:
		/*WFC - WiFi Connect*/
		WiFi.mode(WIFI_STA);
		WiFi.begin(parametros[0],parametros[1]);
		previousMillis = millis();
		WF_STATUS = WiFi.status();
		while (WF_STATUS != WL_CONNECTED) {
			currentMillis = millis();
			if((currentMillis - previousMillis) > MAX_WIFICONNECT_TO) {
				WFC_STATUS = 0;
				break;
			}
			WF_STATUS = WiFi.status();
			delay(20);
		}
/*		if(WFC_STATUS==1){
			Serial.println("OK");
			break;
		}else{
			Serial.println("E2");
			break;
		}*/
		switch(WF_STATUS){
		case WL_IDLE_STATUS:
			Serial.println("E7");
			break;
		case WL_NO_SSID_AVAIL:
			Serial.println("E4");
			break;
		case WL_SCAN_COMPLETED:
			Serial.println("E5");
			break;
		case WL_CONNECT_FAILED:
			Serial.println("E3");
			break;
		case WL_CONNECTION_LOST:
			Serial.println("E6");
			break;
		case WL_DISCONNECTED:
			Serial.println("E1");
			break;
		case WL_CONNECTED:
			Serial.println("OK");
			break;
		default:
			break;
		}
		break;
	case 1:
		/*WFS - WiFi Scan*/
		WiFi.mode(WIFI_STA);
		WiFi.disconnect();
		delay(100);
		numSsid = WiFi.scanNetworks();
		if (numSsid == -1) {
		Serial.println("NC");
		}else{
			// print the list of networks seen:
			Serial.println(numSsid);

			// print the network number and name for each network found:
			for (int thisNet = 0; thisNet < numSsid; thisNet++) {
			Serial.print(thisNet);
			Serial.print(") ");
			Serial.print(WiFi.SSID(thisNet));
			//Serial.printf("SSID: %s", WiFi.SSID().c_str());
			Serial.print("\tSignal: ");
			Serial.print(WiFi.RSSI(thisNet));
			Serial.println(" dBm");
			}
		}
		break;
	case 2:
		/*WRI - WiFi RSSI*/
		Serial.println(WiFi.RSSI());
		break;
	case 3:
		/*WID - WiFi ID*/
		Serial.println(WiFi.SSID());
		break;
	case 4:
		/*WFD - WiFi Disconnect*/
		WiFi.disconnect();
		delay(100);
		Serial.println("OK");
		break;
	case 5:
		/*WCF - WiFi Configuration*/
		if(!ip.fromString(parametros[0])){
			/*IP Invalida*/
			Serial.println("E1");
			break;
		}
		if(!dns.fromString(parametros[1])){
			/*DNS Invalido*/
			Serial.println("E2");
			break;
		}
		if(!gateway.fromString(parametros[2])){
			/*Gateway Invalido*/
			Serial.println("E3");
			break;
		}
		if(!subnet.fromString(parametros[3])){
			/*Subnet Invalido*/
			Serial.println("E4");
			break;
		}
		if(WiFi.config(ip,gateway,subnet,dns)){
			Serial.println("OK");
		}else{
			Serial.println("E");
		}

		break;
	case 6:
		/*CCS - Client Connect to Server*/
		port = atoi(parametros[1]);
		Serial.println(port);
		if(!inRange(port,0,MAX_PORT_NUMBER)){
			Serial.println("E1");
			break;
		}
		WF_STATUS = WiFi.status();
		if( (WF_STATUS == WL_DISCONNECTED) || (WF_STATUS == WL_CONNECTION_LOST) ){
			Serial.println("E2");
			break;
		}
		socket = getFreeSocket();
		C_STATUS = client[socket].connect(parametros[0],port);
		if(C_STATUS){
			Serial.print("OK");
			Serial.print(",");
			Serial.println(socket);
		}else{
		}
		break;
	case 7:
		/*SOW - Socket Write*/
		/*Verificar primero si existe una conexion activa antes de intentar enviar el mensaje*/
		socket = atoi(parametros[0]);
		bytesToWrite = atoi(parametros[1]);
		if(inRange(socket,0,MAX_NUM_CLIENTS) == true){
			if( (bytesToWrite >= 0) && (bytesToWrite <= MAX_PACKET_SIZE) ){
				if(client[socket].connected()){
					/*data to print: char, byte, int, long, or string*/
					/*The max packet size in TCP is 1460 bytes*/
					bytesWritten = client[socket].write(parametros[2],bytesToWrite);
					/*Waits for the TX WiFi Buffer be empty.*/
					client[socket].flush();
					if(bytesToWrite != bytesWritten){
						Serial.println("E");
					}else{
						Serial.println("OK");
					}
				}else{
					Serial.println("NC");
				}
			}else{
				Serial.println("IB");
			}
		}else{
			Serial.println("IS");
		}
		break;
	case 8:
		/*SOR - Socket Read*/
		socket = atoi(parametros[0]);
		/*print received data from server*/
		if(!inRange(socket,0,MAX_NUM_CLIENTS)){
			Serial.println("IS");
			break;
		}
		Serial.write(bufferReceivedFromServer[socket]);
		bytesReceivedFromServer[socket] = 0;
		/*Clear the buffer*/
		for (int i=0; i < MAX_PACKET_SIZE; i++){
			bufferReceivedFromServer[socket][i] = 0;
		}
		fullBufferRcvd[socket] = false;
		break;
	case 9:
		/*SOC- Socket Close*/
		socket = atoi(parametros[0]);
		if(!inRange(socket,0,MAX_NUM_CLIENTS) == true){
			Serial.println("IS");
			break;
		}
		client[socket].stop();
		Serial.println("OK");
		break;
	case 10:
		/*SLC - Server Listen to Clients*/
		port = atoi(parametros[0]);
		/*Determinar primero si el puerto es valido*/
		if(!inRange(port,0,MAX_PORT_NUMBER)){
			Serial.println("IP");
			break;
		}
		/*Determinar si ya existe un servidor funcionando con ese puerto*/
		for (i = 0; i < MAX_NUM_SERVERS; i++) {
			if(port != serverPortsInUse[i]){
				portInUse = false;
				break;
			}else{
				portInUse = true;
				break;
			}
		}
		/*Si no existe un servidor con ese puerto, determinar que servidor esta libre y crear el servidor*/
		if(!portInUse){
			serverPortsInUse[i] = port;
			for (i = 0; i < MAX_NUM_SERVERS; i++) {
				if(server[i].status() == CLOSED){
					server[i].begin(port);
					Serial.print("OK,");
					Serial.println(i,DEC);
					serverOn[i] = true;
					break;
				}
			}
		}else{
			Serial.println("UP");
		}
		break;
	case 11:
		/*SCC - Server Close Connection*/
		socket = atoi(parametros[0]);
		if(!inRange(socket,0,MAX_NUM_SERVERS)){
			Serial.println("IS");
			break;
		}
		server[socket].stop();
		serverOn[socket] = false;
		Serial.println("OK");
		break;
	case 12:
		/*SAC - Server Accept Clients*/
		socket = atoi(parametros[0]);
		if(!inRange(socket,0,MAX_NUM_SERVERS)){
			Serial.println("IS");
			break;
		}
		if(serverOn[socket]){
			if(server[socket].hasClient()){
				acceptClients(server[socket]);
			}else{
				Serial.println("NC");
			}
		}else{
			Serial.println("SOFF");
		}
		break;
	case 13:
		socket = atoi(parametros[0]);
		if (client[socket]) {
		  client[socket].stop();
		}
		Serial.println("Closed.");
		break;
	case 14:
		/*GFH - Get Free Heap*/
		Serial.println(ESP.getFreeHeap(),DEC);
		break;
	case 15:
		/*MIS - Module Is Alive*/
		Serial.println("OK");
		break;
	default:
		break;
	}
}

/* Descripcion: Recibe los datos seriales, byte por byte*/
void recvWithEndMarker() {
	char recvChar, dump;
	char bufferCmd[4] = {'\0'};
	char bufferNumBytes[4] = {'\0'};
	char tempBuffer[MAX_PACKET_SIZE];
	static uint16_t indxRecv = 0;
	uint16_t indxComa1=0, indxComa2=0, qComa=0, indxData = 0;
	uint16_t cmdPacketSize = 0;
	bool dataCmd = false, Coma2 = false, numBytes = false, runDataCmd = false;

	/*Mientras haya datos serial disponibles*/

	while (Serial.available() > 0 && newData == false) {

		/*Recibe caracter por caracter y lo alamcena en un buffer*/
		recvChar = Serial.read();
		tempBuffer[indxRecv] = recvChar;

		/*Se pregunta si se recibio el comando de datos(SOW)*/
		if(indxRecv == 2){
			memcpy(bufferCmd,&tempBuffer,3);
			bufferCmd[3] = '\0';
			if(!strcmp(bufferCmd,"SOW")){
				dataCmd = true;
			}else{
				dataCmd = false;
			}
		}

		/*Se lleva la cuenta de las comas y se guarda la posicion de la segunda y tercera*/
		if(recvChar == ','){
			if(qComa == 1){
				indxComa1 = indxRecv;
			}
			if(qComa == 2){
				indxComa2 = indxRecv;
				Coma2 = true;
			}
			qComa++;
		}

		/*Si aparecio el comando SOW y se alcanzo la cantidad de comas, se verifica que la cantidad de bytes
		 * este presente y sea un numero valido*/
		if((dataCmd == true && Coma2 == true) && (numBytes == false)){
			memcpy(bufferNumBytes,&tempBuffer[indxComa1+1],(indxComa2 - indxComa1 - 1));
			bufferNumBytes[indxComa2 - indxComa1] = '\0';
			cmdPacketSize = atoi(bufferNumBytes);
			if(inRange(cmdPacketSize,0,MAX_PACKET_SIZE)){
				numBytes = true;
				runDataCmd = true;
			}
		}

		/*Si se valido el comando y la cantidad a transmitir, se espera hasta que lleguen los
		 * datos y se envia*/
		if((runDataCmd == true) && (indxRecv > indxComa2)){
			if(indxData < cmdPacketSize){
				serialCharsBuffer[indxRecv] = recvChar;
				indxRecv++;
				indxData++;
			}else{
				if((recvChar == endMarker)){
					serialCharsBuffer[indxRecv] = '\0'; // terminate the string
					indxRecv = 0;
				}else{
					/*Si entra aca significa que no se termino el comando con LF(\n)*/
					serialCharsBuffer[0] = '\0'; // terminate the string
					indxRecv = 0;
				}
				/*Se vacia el buffer serial, necesario para no procesar basura*/
				while (Serial.available() > 0){
					dump = Serial.read();
				}
				newData = true;
			}
		}else{
			if (recvChar != endMarker) {
				serialCharsBuffer[indxRecv] = recvChar;
				indxRecv++;
				if (indxRecv >= numChars) {
					indxRecv = numChars - 1;
				}
			}else{
				serialCharsBuffer[indxRecv] = '\0'; // terminate the string
				indxRecv = 0;
				newData = true;
			}
		}
	}
}

/* Descripcion: Muestra los datos recibidos por serial*/
void showNewData() {
	if (newData == true) {
		Serial.print("Recibido:");
		Serial.println(serialCharsBuffer);
	}
}

/* Descripcion: Separa los datos recibidos en campos*/
void parseData() {
	int i;
	char delim[2];
	/*Copia el caracter que separa los parametros*/
	strcpy(delim,delimiter);
	/*Puntero para indicar donde se encuentra la cadena*/
	char * strtokIndx; // this is used by strtok() as an index
	/*Obtiene la primera parte, la instruccion*/
	strtokIndx = strtok(tempChars,delim);
	/*Mientras no encuentre el fin de la cadena*/
	while(strtokIndx != NULL){
	  strcpy(bufferSerial[delimFound],strtokIndx);
	  strtokIndx = strtok(NULL, delim);
	  delimFound++;
	}
	if(delimFound > 0){
		parametersFound = delimFound - 1;
	}
	strcpy(INST,bufferSerial[0]);
	for(i=0; i < parametersFound; i++){
		strcpy(parametros[i],bufferSerial[i+1]);
	}
}

/* Descripcion: Muestra los datos separados en campos */
void showParsedData() {
	int i;
	Serial.print("Encontre: ");
	Serial.print(delimFound-1,DEC);
	Serial.println(" Comas");
	Serial.print("Instruccion: ");
	Serial.println(INST);
	Serial.println("Parametros: ");
	for(i=0; i < parametersFound; i++){
		Serial.print(i+1,DEC);
		Serial.print("-");
		Serial.println(parametros[i]);
	}
}

/* Descripcion: Recibe un byte del servidor*/
void receiveFromServer(){
	uint16_t bytesAvailable;
	char dump;
	for(int i = 0; i < MAX_NUM_CLIENTS; i++){
		if(client[i].connected()){
			/*Retorna la cantidad de bytes disponibles*/
			bytesAvailable = client[i].available();
			/*Si hay bytes disponibles y el buffer no esta lleno*/
			if (bytesAvailable) {
				if(!fullBufferRcvd[i]){
					//Serial.println("Leido");
					/*Lee el siguiente byte recibido*/
					bufferReceivedFromServer[i][bytesReceivedFromServer[i]++] = client[i].read();
					/*Si la cantidad de bytes leidos por el servidor supero el tamaño
					 * del buffer, se indica que se lleno el buffer.*/
					if(bytesReceivedFromServer[i] > MAX_PACKET_SIZE){
						//Serial.println("Me llene");
						fullBufferRcvd[i] = true;
					}
				}else{
					//Serial.println("Dump");
					dump = client[i].read();

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
uint8_t	getFreeSocket(){
	uint8_t i=0;
	for (i = 0; i < MAX_NUM_CLIENTS; i++) {
		/*Si esta vacio y no esta conectado*/
		if (!client[i] || !client[i].connected()) {
			if (client[i]) {
			  client[i].stop();
			}
			return i;
		}
	}
	/*Si no hay ningun lugar disponible*/
	if (i == MAX_NUM_CLIENTS) {
		return 255;
	}
	return 255;
}

/* Descripcion: Se encarga de aceptar a clientes que se quieren conectar al servidor*/
void acceptClients(WiFiServer& server){
	uint8_t socket;
	socket = getFreeSocket();
	if(socket!=255){
		/*Se encontro socket libre*/
		client[socket] = server.available();
		Serial.print("OK,");
		Serial.println(socket,DEC);
	}else{
		/*No hay socket disponible*/
		WiFiClient serverClient = server.available();
		serverClient.stop();
		Serial.println("NS");
	}
}



