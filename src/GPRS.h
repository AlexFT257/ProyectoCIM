
#define TINY_GSM_MODEM_SIM808

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

const char server[]   = "mongo-arduino-cim.taicrosxy.workers.dev";
const char resource[] = "/api/ArduinoData";
const int  port       = 80;

HardwareSerial Serial_SIM_Module(1);

TinyGsm sim808(Serial_SIM_Module);
TinyGsmClient client(sim808);
HttpClient    http(client, server, port);  

bool networkConnect() {
    Serial.print("Esperando red...");
    if (!sim808.waitForNetwork()) {
        Serial.println("Falló el modem");
        return false;
    }

    if (sim808.isNetworkConnected()) {
        Serial.println("----> Network connected");
    }

    Serial.print("Conectando a ");
    Serial.print(apn);

    if (!sim808.gprsConnect(apn, user, pass)) {
        Serial.println("Falló el GPRS");
        return false;
    }

    if (sim808.isGprsConnected()) {
        Serial.println("----> GPRS connected");
    }

    return true;
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

int sendHttpQuery() {
    int statusCode = 500;
    Serial.println("Intentando realizar conexion a la base de datos...");
    
    //The R() is to write json easier
    char json[] = char json[] = R"(
    {
      "arduinoData": {
        "type": "Position",
        "data": [
          {
            "latitude": 3,
            "longitude": 3
          },
          {
            "latitude": 4,
            "longitude": 4
          }
        ]
      }
    }
    )";
    
    http.beginRequest(); 
    
    http.post(resource);
    http.sendHeader("authorization", "pURzWbUHfRPJrOKoRTFnbTCrFAeBixKDmgjOJ85HqRDp47VWvSo1E2hsvZLjheCr");
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", strlen(json));
    http.beginBody();
    http.print(json);
    
    http.endRequest();
    
    delay(3000);
    
    // if (err != 0)
    // {
    //     Serial.println(F("failed to connect"));
    //     delay(10000);
    //     return 400;
    // }
    
    int status = http.responseStatusCode();
    Serial.print(F("Codigo de respuesta: "));
    Serial.println(status);
    if (status <= 0) {
        delay(1000);
        return 400;
    }

    Serial.println(F("Headers:"));
    while (http.headerAvailable()) {
        String headerName = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        Serial.println("    " + headerName + " : " + headerValue);
    }

    /* int length = http.contentLength();
    if (length >= 0) {
        Serial.print(F("Content length is: "));
        Serial.println(length);
    }
    if (http.isResponseChunked()) {
        Serial.println(F("The response is chunked"));
    } */

    String body = http.responseBody();
    Serial.println(F("Respuesta:"));
    Serial.println(body);

    /* Serial.print(F("Body length is: "));
    Serial.println(body.length()); */


    http.stop();
    Serial.println(F("Servidor desconectado"));

    // Close the connection
    client.stop();

    Serial.println("Comunicacion con la base de datos exitosa");
    return status;
}

bool getGPSPos(float* latitud, float* longitud) {
    float lat, lon;

    // Intenta obtener los valores de GPS
    if(sim808.getGPS(&lat, &lon))  {  // Obtiene la primera posicicon
        *latitud = lat;
        *longitud = lon;

        return true;
    } else {
        return false;
    }   
}
