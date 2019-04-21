#include "Arduino.h"
#include "cmd_definicion.h"
#include "procesador_comandos.h"
/*Considerar usar cliente_tcp.setNoDelay para desactivar el algoritmo de naggle*/

void setup()
{
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
    Serial.print("R");
    Serial.print(CMD_TERMINATOR);
}

void loop()
{
	cmd_procesador();
}


