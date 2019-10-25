# Nuevo conjunto de comandos para el SoC ESP8266
## Descripción
Este firmware implementa un nuevo conjunto de comandos para el ESP8266, para que este pueda ser controlado por un microcontrolador externo, utilizando el periférico UART a una velocidad de 115200 bps. 
## Resumen de comandos

En las tablas [\[tab:comandos\_basicos\]](#tab:comandos_basicos),
[\[tab:comandos\_wifi\]](#tab:comandos_wifi) y
[\[tab:coomnados\_tcpip\]](#tab:coomnados_tcpip) se presentan todos los
comandos disponibles y una breve descripción de la función de cada uno.

### Comandos Básicos

| **Comando** | **Descripción**                                          |
| :---------- | :------------------------------------------------------- |
| MIS         | Verifica el funcionamiento del módulo.                   |
| MRS         | Reinicia el módulo.                                      |
| MVI         | Retorna la versión actual del *firmware*.                |
| MDS         | Configura el modo de bajo consumo *deep-sleep*.          |
| MUC         | Configura la velocidad de transmisión UART.              |
| MRP         | Configura la potencia de transmisión RF.                 |
| MFH         | Retorna cantidad de bytes disponibles en la memoria RAM. |


### Comandos WiFi

| **Comando** | **Descripción**                                          |
| :---------- | :------------------------------------------------------- |
| WFM         | Establece el modo WiFi del módulo.                       |
| WFC         | Conecta a un AP en modo estación.                        |
| WFS         | Retorna el SSID de los puntos de acceso en rango.        |
| WFD         | Desconecta al módulo del AP.                             |
| WFA         | Configura el módulo como un AP.                          |
| WSI         | Retorna información acerca de la interfaz AP.            |
| WAD         | Desactiva el AP del módulo.                              |
| WCF         | Desactiva DHCP en modo estación, configuración estática. |
| WAC         | Configura los parámetros de red del AP.                  |
| WMA         | Configura la dirección MAC del módulo.                   |
| WSC         | Inicia SmartConfig.                                      |
| WSS         | Detiene SmartConfig.                                     |
| WSD         | Retorna estado SmartConfig.                              |
| WSN         | Configura el *hostname* del módulo.                      |
| WRI         | Retorna el RSSI.                                         |
| WID         | Retorna el SSID del AP al cual se encuentra conectado.   |


### Comandos TCP/IP.

| **Comando** | **Descripción**                                           |
| :---------- | :-------------------------------------------------------- |
| SOI         | Retorna información acerca de una conexión.               |
| IDN         | Retorna la dirección IP a partir de un nombre de dominio. |
| CCS         | Inicia una conexión, en modo cliente.                     |
| SOW         | Envía datos a través de una conexión TCP.                 |
| SOR         | Recibe datos a través de una conexión TCP.                |
| SOC         | Cierra una conexión TCP.                                  |
| WFI         | Retorna la configuración de red actual del modo estación. |
| WAI         | Retorna la configuración de red actual del modo AP.       |
| SLC         | Crea un servidor TCP en el módulo.                        |
| SAC         | Acepta clientes que desean conectarse a un servidor.      |
| SCC         | Detiene un servidor TCP.                                  |
| SVU         | Crea un servidor UDP.                                     |
| SDU         | Envía datos utilizando el protocolo UDP.                  |
| RVU         | Recibe datos utilizando el protocolo UDP.                 |
| STC         | Configura el servidor SNTP.                               |
| STG         | Obtiene el tiempo actual del servidor SNTP.               |


