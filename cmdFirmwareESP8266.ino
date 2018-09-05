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
#define MAX_PARAMETERS 3

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define MAX_CHARS_PARAMETERS 1024

/*Cantidad maxima de caracteres que puede contener el comando*/
#define MAX_CHAR_INST 3

/*Numero maximo de instrucciones*/
#define MAX_INTS_SET 15

/*---------------------*/
/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define MAX_SERVER_TO 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define MAX_WIFICONNECT_TO 20000

/*Maximum number of Bytes for a packet*/
#define MAX_PACKET_SIZE 512

/*Numero maximo de clientes que puede manejar el modulo*/
#define MAX_NUM_CLIENTS 4

#define MAX_NUM_SERVERS 4

/**/
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
bool	SERVER_ON = false;

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
		"GFH"};	//14

/*Matriz que almacena la cantidad de parametros necesarios
 *por cada comando, correspondencia por indice.*/
const uint8_t qParametersInstruction[MAX_INTS_SET] ={2,0,0,0,0,3,2,3,1,1,1,0,0,1,0};

/*Declaracion del objeto que se utilizara para el manejo del cliente, maximo 4
 * por limitacion del modulo.*/
WiFiClient client[MAX_NUM_CLIENTS];

/*Declaracion de los objetos que se utilizaran para el manejo del servidor*/

WiFiServer server1(DEFAULT_SERVER_PORT);
WiFiServer server2(DEFAULT_SERVER_PORT);
WiFiServer server3(DEFAULT_SERVER_PORT);
WiFiServer server4(DEFAULT_SERVER_PORT);

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
	for (int i = 0; i < strlen(serialCharsBuffer); ++i) {
		Serial.printf("%02x ", serialCharsBuffer[i]);
	}

	if(newData == true){
		Serial.println(serialCharsBuffer);
		/*Serial.print("H:");
		Serial.println(ESP.getFreeHeap());*/
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
		showParsedData();

		yield();

		if(searchInstruction() == true){
			if(isCommand() == true){
				if(validateParameters() == true){
					runInstruction();
				}else{
					Serial.println("NEP");
				}
			}else{
				Serial.println("DATA");
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
	/**/
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
	Serial.println("Entro search");
	for(j=0; j<MAX_INTS_SET; j++){
		Serial.print(j,DEC);
		if(strcmp(INST,instructionSet[j]) == 0){
			instructionIndex = j;
			Serial.println("retorno ok");
			return 1;

		}
	}
	Serial.println("retorno o");
	return 0;
}

/* Descripcion: Comprueba si el comando recibido es un comando o dato.*/
bool isCommand(){


	return 1;
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
	int port;
	int bytesToWrite, bytesWritten;
	int numSsid;
	uint8_t socket;
	uint8_t i;
	bool WFC_STATUS = 1, portInUse = false;
	if(searchInstruction() && validateParameters()){
		switch(instructionIndex){
		case 0:
			/*WFC - WiFi Connect*/
			WiFi.mode(WIFI_STA);
			Serial.println("A");
			WiFi.begin(parametros[0],parametros[1]);
			/*for (int i = 0; i < strlen(parametros[0]); ++i) {
				  Serial.printf("%02x ", parametros[0][i]);
			  }
			Serial.println("");
			for (int i = 0; i < strlen(parametros[1]); ++i) {
				Serial.printf("%02x ", parametros[1][i]);
			}*/
			//Serial.println("");
			Serial.println("B");
			previousMillis = millis();
			Serial.println("Mbo");
			while (WiFi.status() != WL_CONNECTED) {
				delay(20);
				currentMillis = millis();
				if((currentMillis - previousMillis) > MAX_WIFICONNECT_TO) {
					WFC_STATUS = 0;
					break;
				}
			}
			if(WFC_STATUS==1){
				Serial.println("OK");
			}else{
				Serial.println("TO");
			}
			break;
		case 1:
			/*WiFi Scan*/
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
			/*WiFi RSSI*/
			Serial.println(WiFi.RSSI());
			break;
		case 3:
			/*WiFi ID*/
			Serial.println(WiFi.SSID());
			break;
		case 4:
			/*WiFi Disconnect*/
			WiFi.disconnect();
			delay(100);
			Serial.println("OK");
			break;
		case 5:
			/*IPAddress addr;
			if (addr.fromString(strIP)) {
			  // it was a valid address, do something with it
			}*/
			/*WiFi Configuration*/
			/*IPAddress local_ip();
			WiFi.config();*/

			break;
		case 6:
			/*CCS - Client Connect to Server*/
			port = atoi(parametros[1]);
			for (i = 0; i < MAX_NUM_CLIENTS; i++) {
				//find free/disconnected spot
				//Si esta vacio y no esta conectado
				if (!client[i] || !client[i].connected()) {
					if (client[i]) {
						client[i].stop();
					}
					if(client[i].connect(parametros[0],port)){
						Serial.print("OK");
						Serial.print(",");
						Serial.println(i);
					}else{
						Serial.println("E");
					}
					break;
				}
			}
			//no free/disconnected spot so reject
			if (i == MAX_NUM_CLIENTS) {
				Serial.println("NS");
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
						//client[socket].println(parametros[2]);
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
			}
			break;
		case 8:
			/*CRS - Client Receive from Server*/
			socket = atoi(parametros[0]);
			/*print received data from server*/
			if(inRange(socket,0,MAX_NUM_CLIENTS) == true){
				Serial.println(bufferReceivedFromServer[socket]);
				bytesReceivedFromServer[socket] = 0;
				/*Clear the buffer*/
				for (int i=0; i < MAX_PACKET_SIZE; i++){
					bufferReceivedFromServer[socket][i] = 0;
				}
				//memset(bufferReceivedFromServer,0,sizeof(bufferReceivedFromServer));
				//bufferReceivedFromServer[0] ='\0';
				/**/
				fullBufferRcvd[socket] = false;
			}
			break;
		case 9:
			/*CCC - Client Close Connection*/
			socket = atoi(parametros[0]);
			if(inRange(socket,0,MAX_NUM_CLIENTS) == true){
				client[socket].stop();
				Serial.println("OK");
			}
			break;
		case 10:
			/*SCL - Server listens to clients*/
			port = atoi(parametros[0]);
			/*Determinar primero si el puerto es valido*/
			if(inRange(port,0,MAX_PORT_NUMBER)){
				SERVER_ON = true;
			}else{
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
				if(server1.status() == CLOSED){
					server1.begin(port);
					Serial.println("OK,0");
					break;
				}
				if(server2.status() == CLOSED){
					server2.begin(port);
					Serial.println("OK,1");
					break;
				}
				if(server3.status() == CLOSED){
					server3.begin(port);
					Serial.println("OK,2");
					break;
				}
				if(server4.status() == CLOSED){
					server4.begin(port);
					Serial.println("OK,3");
					break;
				}
			}else{
				Serial.println("UP");
			}
			break;
		case 11:
			/*SCC*/
			SERVER_ON = false;
			server1.stop();
			Serial.println("OK");
			break;
		case 12:
			/*SAC*/
			if(SERVER_ON == true){
				/*Verifica si el servidor tiene clientes esperando*/
			  if (server1.hasClient()) {
				  acceptClients(server1);
			  }else{
				  /*El servidor no tiene clientes esperando*/
					Serial.println("NC");
			  }

			  if (server2.hasClient()) {
				  acceptClients(server2);
			  }else{
				  /*El servidor no tiene clientes esperando*/
					Serial.println("NC");
			  }

			  if (server3.hasClient()) {
				  acceptClients(server3);
			  }else{
				  /*El servidor no tiene clientes esperando*/
					Serial.println("NC");
			  }

			  if (server4.hasClient()) {
				  acceptClients(server4);
			  }else{
				  /*El servidor no tiene clientes esperando*/
					Serial.println("NC");
			  }

			}else{
				/*El servidor esta descativado*/
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
		default:
			break;
		}

	}

}

/* Descripcion: */
void recvWithEndMarker() {
	char recvChar, dump = '\0';
	char bufferCmd[4] = {'\0'};
	char bufferNumBytes[4] = {'\0'};
	char tempBuffer[MAX_PACKET_SIZE];
	static uint16_t indxRecv = 0;
	uint16_t indxComa1=0, indxComa2=0, qComa=0, indxData = 0;
	uint16_t cmdPacketSize = 0;
	bool dataCmd = false, Coma2 = false, numBytes = false, runDataCmd = false;
	unsigned long previousTime = 0;
	unsigned long currentTime = 0;

	/*Mientras haya datos serial disponibles*/

	while (Serial.available() > 0 && newData == false) {

		/*Recibe caracter por caracter y lo alamcena en un buffer*/
		recvChar = Serial.read();
		//Serial.println(indxRecv,DEC);
		tempBuffer[indxRecv] = recvChar;

		/*Se pregunta si se recibio el comando de datos(SOW)*/
		if(indxRecv == 2){
			Serial.println("Indice:2");
			memcpy(bufferCmd,&tempBuffer,3);
			bufferCmd[3] = '\0';
			Serial.println(bufferCmd);
			if(!strcmp(bufferCmd,"SOW")){
				Serial.println("SOW APARECIO");
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
			Serial.println("\n");
			Serial.println(bufferNumBytes);
			cmdPacketSize = atoi(bufferNumBytes);
			if(inRange(cmdPacketSize,0,MAX_PACKET_SIZE)){
				numBytes = true;
				runDataCmd = true;
			}
			Serial.print("Packet Size:");
			Serial.println(cmdPacketSize,DEC);
			/*Agregar bandera aqui para que haga solo una vez*/
			//indxData = indxComa2 + 1;
		}

		/*Si se valido el comando y la cantidad a transmitir, se espera hasta que lleguen los
		 * datos y se envia*/
		if((runDataCmd == true) && (indxRecv > indxComa2)){
			Serial.print(indxData);
			Serial.print(":");
			Serial.print(recvChar);
			Serial.print(" ");
			if(indxData < cmdPacketSize){
				serialCharsBuffer[indxRecv] = recvChar;
				Serial.println(indxData,DEC);
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
					Serial.println("Dumped");
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
	 //newData = false;
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
	Serial.println("While Parser");
	while(strtokIndx != NULL){
	  strcpy(bufferSerial[delimFound],strtokIndx);
	  strtokIndx = strtok(NULL, delim);
	  delimFound++;
	}
	Serial.println("Salida Parser");
	if(delimFound > 0){
		parametersFound = delimFound - 1;
	}
	Serial.println(parametersFound,DEC);
	strcpy(INST,bufferSerial[0]);
	Serial.println("Entra al for");
	for(i=0; i < parametersFound; i++){
		strcpy(parametros[i],bufferSerial[i+1]);
	}
	Serial.println("Salida funcion");
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

bool inRange(uint16_t val, uint16_t min, uint16_t max)
{
  return ((min <= val) && (val <= max));
}

uint8_t	getFreeSocket(){
	uint8_t i=0;
	for (i = 0; i < MAX_NUM_CLIENTS; i++) {
		//Si esta vacio y no esta conectado
	  if (!client[i] || !client[i].connected()) {
		if (client[i]) {
		  client[i].stop();
		}
		return i;
	  }
	}
	//no free/disconnected spot so reject
	if (i == MAX_NUM_CLIENTS) {
		return 255;
	}
}

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



