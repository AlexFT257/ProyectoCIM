
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

const char server[] = "mongo-arduino-cim.taicrosxy.workers.dev";
const char resource[] = "/api/ArduinoData";
const int port = 80;

const int id = 1;  // TODO CAMBIAR PARA CADA MAQUINA

// mongoEndpointAPIKEY =  hDYzA5V8btEmWF0tH1Pe1E6MVolfd5QSzaVCmJjOaOxcGl9WUNdrW0bB54mHn3m8
HardwareSerial Serial_SIM_Module(1);

TinyGsm sim808(Serial_SIM_Module);
TinyGsmClient client(sim808);
HttpClient http(client, server, port);

bool networkConnect() {
    Serial.print("Esperando red...");
    if (!sim808.waitForNetwork()) {
        Serial.println("Falló el modem");
        //return false;
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

String getArduinoDataJson(String data) {
    return R"({ "arduinoData":  )" + data + R"(})";
}

String getPositionJson(float lat, float lon, int type) {
    return R"(
        {
            "type": "Position",
            "data": {
                "latitude": )" +
           String(lat) + R"(,
                "longitude": )" +
           String(lon) + R"(,
                "type": )" +
           String(type) + R"(
            }
        }
        
    )";
}

String getStadisticsJson(String statFile) {
    File file = SD.open(statFile.c_str(), FILE_READ);
    String data = "";

    // PUEDE FALLAR EL BUFFER
    if (file) {
        while (file.available()) {
            data += file.readStringUntil('\n')+"\n";
        }
        file.close();
    }

    Serial.println(data);

    String json = R"(
        {
            "type": "Stadistics",
            "data": [)";

    int index = 0;

    while (index < data.length()) {
        int semicolonIndex = data.indexOf(";", index);
        int newlineIndex = data.indexOf("\n", index);

        if (semicolonIndex == -1 || newlineIndex == -1) {
            break;
        }

        String date = data.substring(index, semicolonIndex);
        String type = data.substring(semicolonIndex + 1, newlineIndex);
        json += R"( { "date": ")" + date + R"(", "type": )" + type + R"( })" + (newlineIndex + 1 >= data.length() ? "" : ",");

        index = newlineIndex + 1;
    }

    json += R"(] } )";

    return json;
}

String getStatusJson(String status) {
    return R"(
        {
            "type": "Status",
            "data": {
                "info": ")" +
           status + R"("
            }
        }
        
    )";
}

int sendHttpQuery(String json) {
    Serial.println("Intentando realizar conexion a la base de datos...");

    http.beginRequest();
    if (json == "") {
        http.get(resource);
        http.sendHeader("authorization", "pURzWbUHfRPJrOKoRTFnbTCrFAeBixKDmgjOJ85HqRDp47VWvSo1E2hsvZLjheCr");
    } else {
        http.post(resource);
        http.sendHeader("authorization", "pURzWbUHfRPJrOKoRTFnbTCrFAeBixKDmgjOJ85HqRDp47VWvSo1E2hsvZLjheCr");
        http.sendHeader("Content-Type", "application/json");
        http.sendHeader("Content-Length", json.length());
        http.beginBody();
        http.print(json);
    }

    http.endRequest();

    delay(3000);

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

    String body = http.responseBody();
    Serial.println(F("Respuesta:"));
    Serial.println(body);

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
    if (sim808.getGPS(&lat, &lon)) {  // Obtiene la primera posicicon
        *latitud = lat;
        *longitud = lon;

        return true;
    } else {
        return false;
    }
}
