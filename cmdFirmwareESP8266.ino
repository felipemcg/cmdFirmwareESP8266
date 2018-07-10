/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include "Arduino.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <ESP8266WiFi.h>
/*
#include <ESP8266WiFiScan.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
    //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
*/

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
#define qInstructionSet 15

/*---------------------*/
/*El tiempo maxima para esperar una respuesta del servidor, em ms.*/
#define serverTimeOut 500

/*Tiempo en mili-segundos para esperar a conectarse*/
#define WiFiConnectTO 10000

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
int delimFound = 0;

/*Cantidad de parametros que se encontro*/
int parametersFound = 0;

/*Caracter que termina la instruccion*/
char endMarker = '\n';

//Campo de instruccion, +1 para el NULL al final
char INST[qCharInst+1] = {'\0'};

/*Array donde se almacenan los parametros*/
char parametros[qParamaters][qCharParameters] = {{'\0'}};

/*Indice que indica cual funcion se recibio*/
int	instructionIndex = 999;

/*Paquete para enviar a traves de la red*/
char packet[packetSize];

char	bufferReceivedFromServer[packetSize];
uint16_t bytesReceivedFromServer;
bool	fullBufferRcvd;
bool	SERVER_ON = false;

/*Matriz que almacena los nombres de todas los comandos validos
 * -WFM: WiFi Mode
 * -WFC: WiFi Connect
 * -CCS: Client Connect to Server
 * -PTS: Print to server
 * -PTL: Print to server, adding CR/LF.
 * -CCC: Client Close Connection.
 * -WFG: */

/*dejar con static? Podria afectar la velocidad*/
static const char instructionSet[qInstructionSet][qCharInst+1] = {"WFC",	//0
		"WFS",	//1
		"WRI",	//2
		"WID",	//3
		"WFD",	//4
		"WCF",	//5
		"CCS",	//6
		"CWS",	//7
		"CRS",	//8
		"CCC",	//9
		"SLC",	//10
		"SRC",	//11
		"SWC",	//12
		"SCC",	//13
		""};	//14

char etx = '\x03';

/*Matriz que almacena la cantidad de parametros necesarios
 *por cada comando, correspondencia por indice.*/
int	qParametersInstruction[qInstructionSet] ={2,0,1,0,0,3,2,2,0,0,1,1,2,0};

size_t r;

const uint8_t numChars = 255;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

char tempChars[numChars];


/*Basicamente provee la misma funcionalidad que el socket*/
WiFiClient client[2];
WiFiServer server(80);

int socketInUse[MAX_NUM_CLIENTS] = {0,0,0,0};


/*-------------------------------------------------------------------*/

void setup() {
	Serial.begin(115200);
	delay(10);
	Serial.println();

	bytesReceivedFromServer = 0;
	fullBufferRcvd = false;
	/*WiFiManager wifiManager;
	//first parameter is name of access point, second is the password
	wifiManager.setDebugOutput(false);
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

	if(newData == true){
		Serial.print("H:");
		Serial.println(ESP.getFreeHeap());
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
					Serial.println("NOT ENOUGH PARAMETERS.");
				}
			}else{
				Serial.println("DATA");
			}
		}else{
			Serial.println("NO CMD.");
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
	checkForClients();
}

/*
 * Descripcion: Compara la cadena que contiene el campo de instruccion(INST) con el array
 * de instrucciones(instructionSet). Si encuentra una coincidencia, guarda el indice y la
 * funcion retorna 1.
 * */
bool searchInstruction(){
	int i;
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
	int port;
	int bytesToWrite, bytesWritten;
	int numSsid;
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
			}
			Serial.println("");*/
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

			break;
		case 3:
			/*WiFi ID*/

			break;
		case 4:
			/*WiFi Disconnect*/

			break;
		case 5:
			/*WiFi Configuration*/


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
						Serial.println("OK");
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

			/*CWS - Client Write to Server*/
			/*Verificar primero si existe una conexion activa antes de intentar enviar el mensaje*/
			if(client[0].connected()){
				/*data to print: char, byte, int, long, or string*/
				/*The max packet size in TCP is 1460 bytes*/
				bytesToWrite = atoi(parametros[0]);
				Serial.println(bytesToWrite,DEC);
				bytesWritten = client[0].write(parametros[1],bytesToWrite);
				Serial.println(bytesWritten,DEC);
				/*Waits for the TX WiFi Buffer be empty.*/
				client[0].flush();
				Serial.println("OK");
			}else{
				Serial.println("NC");
			}
			break;
		case 8:
			/*CRS - Client Receive from Server*/

			/*print received data from server*/
			Serial.println(bufferReceivedFromServer);
			/**/
			bytesReceivedFromServer = 0;

			/*Clear the buffer*/
			memset(bufferReceivedFromServer,0,sizeof(bufferReceivedFromServer));
			//bufferReceivedFromServer[0] ='\0';
			/**/
			fullBufferRcvd = false;
			break;
		case 9:
			/*CCC - Client Close Connection*/
			client[0].stop();
			Serial.println("OK");
			break;
		case 10:
			/*SCL - Server listens to clients*/
			SERVER_ON = true;
			port = atoi(parametros[0]);
			server.begin(port);
			Serial.println("SOK");
			// Check if a new client has connected
			  //WiFiClient newClient = server.available();
			  /*if (client) {
				Serial.println("new client");
				// Find the first unused space
				for (int i=0 ; i<2 ; ++i) {
					if (NULL == client[i]) {
						client[i] = new WiFiClient(newClient);
						break;
					}
				 }
			  }*/
			break;
		case 11:
			/*SRC - Server receive from clients*/

			break;
		case 12:
			/*SRC - Server write to clients*/
			break;
		case 13:
			/*SCC - Server close connection*/
			break;
		default:
			break;
		}

	}

}

/* Descripcion: */
void recvWithEndMarker() {
 static byte ndx = 0;

/*Received Byte*/
char rc;

/**/
 while (Serial.available() > 0 && newData == false) {
	 rc = Serial.read();
	 if (rc != endMarker) {
		 receivedChars[ndx] = rc;
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
	  if(delimFound == qParamaters){
		  strcpy(delim,"\n");
	  }
	}
	parametersFound = delimFound - 1;
	strcpy(INST,bufferSerial[0]);
	/*Para sacar el caracter de LF del ultimo parametro.*/
	for(i=0; i < strlen(bufferSerial[delimFound]); i++){
		if(bufferSerial[delimFound][i]== '\n'){
			bufferSerial[delimFound][i] = '\0';
		}
	}
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
	if(client[0].connected()){
		/*Retorna la cantidad de bytes disponibles*/
		int bytesAvailable = client[0].available();
		/*Si hay bytes disponibles y el buffer no esta lleno*/
		if (bytesAvailable && !fullBufferRcvd) {
			/*Lee el siguiente byte recibido*/
			bufferReceivedFromServer[bytesReceivedFromServer++] = client[0].read();
			/*Si la cantidad de bytes leidos por el servidor supero el tamaño
			 * del buffer, se indica que se lleno el buffer.*/
			if(bytesReceivedFromServer > packetSize){
				fullBufferRcvd = true;
			}
		}
	}
}

void checkForClients(){
	uint8_t i;
	if(SERVER_ON == true){
		  //check if there are any new clients
		  if (server.hasClient()) {
			for (i = 0; i < MAX_NUM_CLIENTS; i++) {
			  //find free/disconnected spot
				//Si esta vacio y no esta conectado
			  if (!client[i] || !client[i].connected()) {
				if (client[i]) {
				  client[i].stop();
				}
				client[i] = server.available();
				Serial.print("New client: "); Serial.println(i);
				break;
			  }
			}
			//no free/disconnected spot so reject
			if (i == MAX_NUM_CLIENTS) {
			  WiFiClient serverClient = server.available();
			  serverClient.stop();
			  Serial1.println("Connection rejected ");
			}
		}
	}
}
