// #define TINY_GSM_MODEM_SIM800
#define TINY_GSM_MODEM_SIM808
// #define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_UBLOX
// #define TINY_GSM_MODEM_BG96
// #define TINY_GSM_MODEM_A6
// #define TINY_GSM_MODEM_A7
// #define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266
// #define TINY_GSM_MODEM_XBEE

#include <ArduinoHttpClient.h>
#include <TinyGsmClient.h>  // Biblioteca que configura el módem GSM

/*
ENTEL=====
APN: bam.entelpcs.cl
user: entelpcs
pass: entelpcs
Tipo de autenticación: PAP
Tipo de APN: default,supl

CLARO=====
APN: bam.clarochile.cl
user: clarochile
pass: clarochile

MOVISTAR=====
APN: wap.tmovil.cl
user: web
pass: web
MCC: 730
MNC: 02
Tipo de autenticación: PAP
Tipo de APN: default

WOM=====
APN: internet
Tipo de APN: Default

Segunda configuración – MMS:

Punto de acceso(APN): mms
MMSC: http://3gmms.nextelmovil.cl
Proxy MMS: 129.192.129.104:8080
*/

const char apn[] = "wap.tmovil.cl";  // Tu APN
const char user[] = "web";
const char pass[] = "web";

const char server[]   = "c2ba-190-5-38-105.ngrok-free.app";
const char resource[] = "/";
const int  port       = 80;


HardwareSerial Serial_SIM_Module(1);

TinyGsm sim808(Serial_SIM_Module);
TinyGsmClient client(sim808);
// HttpClient http(client, "https://sa-east-1.aws.data.mongodb-api.com");  // MAYBE NOT NECESARY
HttpClient    http(client, server, port);  // MAYBE NOT NECESARY



bool networkConnect() {

    // if(sim808.addCertificate("C:\\USER\\CERT.CRT")){
    //     Serial.println("Certificate added");
    // }else{
    //     Serial.println("Certificate not added");
    // }

    Serial.print("Esperando red...");
    if (!sim808.waitForNetwork()) {
        Serial.println(" falló");
        return false;
    }

    if (sim808.isNetworkConnected()) {
        Serial.println("Network connected");
    }



    Serial.println(" OK");
    Serial.print("Conectando a ");
    Serial.print(apn);
    if (!sim808.gprsConnect(apn, user, pass)) {
        Serial.println(" falló");
        return false;
    }

    if (sim808.isGprsConnected()) {
        Serial.println("GPRS connected");
    }
    Serial.println(" OK");
    return true;
}

String sendAT(String msg) {
    sim808.sendAT(msg);  // Enviar comando al modem

    String response;
    bool success = sim808.waitResponse(5000, response, "OK", "ERROR");

    if (success) {
        // Response received, process it
        Serial.println("Response: " + response);
    } else {
        // Timeout or unexpected response
        Serial.println("No response or error occurred: "+ response);

    }

    return response;
}

// Configura el módem GPRS
bool modemConfig() {
    // Inicia el módem
    Serial.println("Configurando el módem...");

    if (!sim808.restart())
        return false;

    // Conecta a la red
    return networkConnect();
}