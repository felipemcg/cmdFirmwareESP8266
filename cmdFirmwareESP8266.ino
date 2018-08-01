#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
    //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

/*-------------------------DEFINE's--------------------------------*/
//#define sDebug

/*---	Comandos	---*/

/*Cantidad maxima de parametros por comando*/
#define qParamaters 3

/*Cantidad maxima de caracteres que puede contener cada parametro*/
#define qCharParameters 1024

/*Cantidad maxima de caracteres que puede contener el comando*/
#define qCharInst 3

/*Numero maximo de instrucciones*/
#define qInstructionSet 13

/*---------------------*/
/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define serverTimeOut 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define WiFiConnectTO 20000

/*Maximum number of Bytes for a packet*/
#define packetSize 512

/*Numero maximo de clientes que puede manejar el modulo*/
#define MAX_NUM_CLIENTS 4

/*Puerto por default que el server escuchara*/
#define DEFAULT_SERVER_PORT
/*-------------------------------------------------------------------*/

/*Array de parametros e instruccion*/
char bufferSerial[qParamaters+1][qCharParameters] = {{'\0'}};

/*Delimitador de la instruccion*/
char delimiter[2] = ",";

/*Cantidad de delimitadores que se encontro*/
uint8_t delimFound = 0;

/*Cantidad de parametros que se encontro*/
uint8_t parametersFound = 0;

/*Caracter que termina la instruccion*/
char endMarker = '\n';

//Campo de instruccion, +1 para el NULL al final
char INST[qCharInst+1] = {'\0'};

/*Array donde se almacenan los parametros*/
char parametros[qParamaters][qCharParameters] = {{'\0'}};

/*Indice que indica cual funcion se recibio*/
uint8_t	instructionIndex = 255;

/*Paquete para enviar a traves de la red*/
char packet[packetSize];

char	bufferReceivedFromServer[MAX_NUM_CLIENTS][packetSize];
uint16_t bytesReceivedFromServer[MAX_NUM_CLIENTS];
bool	fullBufferRcvd[MAX_NUM_CLIENTS];
bool	SERVER_ON = false;

/*Matriz que almacena los nombres de todas los comandos validos
/*dejar con static? Podria afectar la velocidad*/
static const char instructionSet[qInstructionSet][qCharInst+1] = {"WFC",	//0
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
		"SAC"};	//12

/*Matriz que almacena la cantidad de parametros necesarios
 *por cada comando, correspondencia por indice.*/
const uint8_t qParametersInstruction[qInstructionSet] ={2,0,0,0,0,3,2,3,1,1,1,0,0};

size_t r;

const uint8_t numChars = 255;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

char tempChars[numChars];


/*Basicamente provee la misma funcionalidad que el socket*/
WiFiClient client[MAX_NUM_CLIENTS];
WiFiServer server(80);

uint8_t socketInUse[MAX_NUM_CLIENTS] = {0,0,0,0};

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
	/*WiFiManager wifiManager;
	//first parameter is name of access point, second is the password
	wifiManager.setDebugOutput(true);
	wifiManager.autoConnect("Nodo_ESP8266");*/

#ifdef sDebug
	Serial.println();
    Serial.println();
    Serial.println("Listo.");
    Serial.println("Las instrucciones que tengo son: ");
    for(int i=0; i < qInstructionSet; i++){
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
	/*for (int i = 0; i < strlen(receivedChars); ++i) {
		Serial.printf("%02x ", receivedChars[i]);
	}*/

	if(newData == true){
		//Serial.println(receivedChars);
		/*Serial.print("H:");
		Serial.println(ESP.getFreeHeap());*/
	#ifdef sDebug
		pMillis = millis();
		/*Echo*/
		showNewData();
	#endif

		/*Se copian los datos recibidos en un array temporal para su manipulacion.*/
		strcpy(tempChars, receivedChars);

		/*Separar los datos en los campos correspondientes.*/
		parseData();

	#ifdef sDebug
		/*Se muestran los datos separados en campos.*/
		showParsedData();
	#endif

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
		memset(receivedChars, 0, sizeof(receivedChars));
		memset(bufferSerial, 0, sizeof(bufferSerial));
		memset(INST, 0, sizeof(INST));
		memset(receivedChars, 0, sizeof(receivedChars));
		memset(parametros, 0, sizeof(parametros));
		delimFound = 0;
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

	checkForClients();
}

/*
 * Descripcion: Compara la cadena que contiene el campo de instruccion(INST) con el array
 * de instrucciones(instructionSet). Si encuentra una coincidencia, guarda el indice y la
 * funcion retorna 1.
 * */
bool searchInstruction(){
	uint8_t i;
	for(i=0; i<qInstructionSet; i++){
		if(strcmp(INST,instructionSet[i]) == 0){
			instructionIndex = i;
			return 1;
		}
	}
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
	int port,dns;
	int bytesToWrite, bytesWritten;
	int numSsid;
	uint8_t socket;
	bool WFC_STATUS = 1;
	if(searchInstruction() && validateParameters()){
		switch(instructionIndex){
		case 0:
			/*WFC - WiFi Connect*/
			WiFi.mode(WIFI_STA);
			WiFi.begin(parametros[0],parametros[1]);
			/*for (int i = 0; i < strlen(parametros[0]); ++i) {
				  Serial.printf("%02x ", parametros[0][i]);
			  }
			Serial.println("");
			for (int i = 0; i < strlen(parametros[1]); ++i) {
				Serial.printf("%02x ", parametros[1][i]);
			}*/
			//Serial.println("");
			previousMillis = millis();
			while (WiFi.status() != WL_CONNECTED) {
				delay(20);
				currentMillis = millis();
				if((currentMillis - previousMillis) > WiFiConnectTO) {
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
			int i;
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
			if(socketIsInRange(socket) == true){
				if( (bytesToWrite >= 0) && (bytesToWrite <= packetSize) ){
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
			}
			break;
		case 8:
			/*CRS - Client Receive from Server*/
			socket = atoi(parametros[0]);
			/*print received data from server*/
			if(socketIsInRange(socket) == true){
				Serial.println(bufferReceivedFromServer[socket]);
				bytesReceivedFromServer[socket] = 0;
				/*Clear the buffer*/
				for (int i=0; i < packetSize; i++){
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
			if(socketIsInRange(socket) == true){
				client[socket].stop();
				Serial.println("OK");
			}
			break;
		case 10:
			/*SCL - Server listens to clients*/
			SERVER_ON = true;
			port = atoi(parametros[0]);
			Serial.print("Estado");
			Serial.println(server.status());
			DEBUGV();
			server.begin(port);
			Serial.println(server.status());
			Serial.println("OK");
			break;
		case 11:
			/*SCC*/
			SERVER_ON = false;
			server.stop();
			Serial.println("OK");
			break;
		case 12:
			/*SAC*/
			if(SERVER_ON == true){
				  //check if there are any new clients
				  if (server.hasClient()) {
					for (uint8_t i = 0; i < MAX_NUM_CLIENTS; i++) {
					  //find free/disconnected spot
						//Si esta vacio y no esta conectado
					  if (!client[i] || !client[i].connected()) {
						if (client[i]) {
						  client[i].stop();
						}
						Serial.println(server.status());
						TCP_DEBUG;
						client[i] = server.available();
						Serial.println(server.status());
						//Serial.print("New client: "); Serial.println(i);
						Serial.print("OK,");
						Serial.println(i,DEC);
						break;
					  }
					}
					//no free/disconnected spot so reject
					if (i == MAX_NUM_CLIENTS) {
					  WiFiClient serverClient = server.available();
					  serverClient.stop();
					  Serial.println("NS");
					  //Serial1.println("Connection rejected ");
					}
				}else{
					Serial.println("NC");
				}
			}else{
				Serial.println("SOFF");
			}
			break;
		default:
			break;
		}

	}

}

/* Descripcion: */
void recvWithEndMarker() {
	static byte ndx = 0;
	bool terminatorFound = false;
	bool dataCmdFound = false;
	bool execDataCmd = false;
	bool firstCommaFound = false;
	bool secondCommaFound = false;
	uint8_t indxCharRcv = 0, indxBytes = 0, indxChar = 0, indxStartData = 0;
	uint16_t numBytes = 0, numBytesCount = 0;
	char dataCmd[qCharInst];
	char numBytesString[4];
	/*Received Byte*/
	char receivedChar,dump;
	while (Serial.available() > 0 && newData == false) {
		/*Se lee un caracter por serial*/
		receivedChar = Serial.read();

		indxCharRcv = ndx;

		/*Serial.print(indxCharRcv,DEC);
		Serial.print("\t");
		Serial.print(receivedChar);
		Serial.print("\t");
*/
		/*Se guardan los primeros caracteres para preguntar por la funcion*/
		if(indxCharRcv < qCharInst){
			dataCmd[indxCharRcv] = receivedChar;
		}
		/*Se pregunta por el comando SOW*/
		if(indxCharRcv == qCharInst){
			dataCmd[indxCharRcv] = '\0';
			if(!strcmp(dataCmd,"SOW")){
				//Serial.print("SOW APARECIO\t");
				dataCmdFound = true;
			}
		}
		if(indxCharRcv == qCharInst + 2){
			if(receivedChar == ',') firstCommaFound=true;
		}
		if(dataCmdFound && firstCommaFound && (secondCommaFound == false) && (indxCharRcv > qCharInst+2)){
			//Serial.println("Bitch");
			if(receivedChar != ','){
				indxChar++;
				if(isDigit(receivedChar)){
					numBytesString[indxBytes++] = receivedChar;
				}else{
				}
			}else{
				secondCommaFound = true;
				if(indxChar == indxBytes){
					numBytesString[indxBytes] = '\0';
					execDataCmd = true;
					indxStartData = indxCharRcv;
					//Serial.print("Cumplio formato de instruccion.\t");
					//Serial.print("");
					//Serial.println(indxData,DEC);
				}else{
					numBytesString[0] = '\0';
				}
				numBytes = atoi(numBytesString);
				/*Serial.print("Numero de bytes enviar: ");
				Serial.print(numBytes,DEC);*/
			}
		}
		if(execDataCmd){
			/*Nunca enviar numero de bytes mayor a la cantidad real, hara que el programa se cuelgue*/
			if(ndx <= (numBytes + indxStartData)){
				//Serial.println("Almaceno contando");
				receivedChars[ndx++] = receivedChar;
				if (ndx >= numChars) {
					ndx = numChars - 1;
				}
			}else{
				receivedChars[ndx] = '\0'; // terminate the string
				ndx = 0;
				/*Serial.print("Ndx: ");
				Serial.print(ndx,DEC);*/
				/*Se vacia el buffer serial*/
				while (Serial.available() > 0){
					//Serial.println("Dumped");
					dump = Serial.read();
				}
				newData = true;
			}
		}else{
			//Serial.println("Almaceno Normal");
			if (receivedChar != endMarker) {
				receivedChars[ndx] = receivedChar;
				ndx++;
				if (ndx >= numChars) {
					ndx = numChars - 1;
				}
			}else{
				receivedChars[ndx] = '\0'; // terminate the string
				ndx = 0;
				newData = true;
			}
		}
	}
	//Serial.println("Sali del while");
}

/* Descripcion: Muestra los datos recibidos por serial*/
void showNewData() {
 if (newData == true) {
	 Serial.print("Recibido:");
	 Serial.println(receivedChars);
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
	while(strtokIndx != NULL){
	  strcpy(bufferSerial[delimFound],strtokIndx);
	  strtokIndx = strtok(NULL, delim);
	  delimFound++;
	}
	parametersFound = delimFound - 1;
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
					if(bytesReceivedFromServer[i] > packetSize){
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

void checkForClients(){

}

bool socketIsInRange(uint8_t socket){
	if( ( socket>=0 ) && ( socket<MAX_NUM_CLIENTS ) ){
		return true;
	}
	Serial.println("ISO");
	return false;
}
