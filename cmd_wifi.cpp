/*
 * cmd_wifi.cpp
 *
 *  Created on: 16 abr. 2019
 *      Author: felip
 */
#include "Arduino.h"
#include "cmd_definicion.h"
#include "utilidades.h"
#include "procesador_comandos.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
bool modo_wifi = false;

/*Comandos para configuracion del WiFi*/
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

void cmd_WCF(){
	IPAddress ip,dns,gateway,subnet;
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);
	if(!ip.fromString(parametros[0])){
		/*IP Invalida*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!dns.fromString(parametros[1])){
		/*DNS Invalido*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(parametros[2])){
		/*Gateway Invalido*/
		Serial.print("3");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(parametros[3])){
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

void cmd_WFC(){
	/*WFC - WiFi Connect*/
	unsigned long millis_anterior;
	unsigned long millis_actual;
	wl_status_t estado_conexion_wifi;
	bool b_conexion_wifi_timeout = 0;
	WiFiMode_t modo_wifi_actual;
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);

	//WiFi.mode(WIFI_STA);
	WiFi.begin(parametros[0],parametros[1]);
	millis_anterior = millis();
	estado_conexion_wifi = WiFi.status();
	while (estado_conexion_wifi != WL_CONNECTED) {
		millis_actual = millis();
		if((millis_actual - millis_anterior) > 20000) {
			b_conexion_wifi_timeout = 1;
			break;
		}
		estado_conexion_wifi = WiFi.status();
		delay(20);
	}
	modo_wifi_actual = WiFi.getMode();
	if(modo_wifi != modo_wifi_actual){
		//Si cambio el modo liberar todos los sockets.
		modo_wifi = modo_wifi_actual;
		//liberar_recursos();
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
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);
	bool b_estacion_off = parametros[0];
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
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);
	if(!ip_local.fromString(parametros[0])){
		/*IP local Invalido*/
		Serial.print("1");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!gateway.fromString(parametros[1])){
		/*Gateway Invalido*/
		Serial.print("2");
		Serial.print(CMD_TERMINATOR);
		return;
	}
	if(!subnet.fromString(parametros[2])){
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

void cmd_WFA(){
	/*WFA - WiFi Soft-AP Mode*/
	bool b_punto_acceso_creado = 0;
	uint8_t canal_wifi = 1;
	uint8_t hidden_opt = 0;
	uint8_t cant_max_clientes_wifi = 4;
	WiFiMode_t modo_wifi_actual;
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);

	canal_wifi = atoi(parametros[2]);
	hidden_opt = atoi(parametros[3]);
	cant_max_clientes_wifi = atoi(parametros[4]);

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
	b_punto_acceso_creado = WiFi.softAP(parametros[0], parametros[1], canal_wifi, hidden_opt, cant_max_clientes_wifi);
	delay(5000);
	modo_wifi_actual = WiFi.getMode();
	if(modo_wifi != modo_wifi_actual){
		//Si cambio el modo liberar todos los sockets.
		modo_wifi = modo_wifi_actual;
		//liberar_recursos();
	}
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
	char parametros[CANT_MAX_PARAMETROS_CMD][CANT_MAX_CARACT_PARAMETRO+1];
	obtener_parametros(&parametros);
	bool b_softAP_off = parametros[0];
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




