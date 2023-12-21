

#define USE_GSM

#include <ArduinoHttpClient.h>
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
user: wap
pass: wap
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

#ifdef USE_GSM
#define TINY_GSM_MODEM_SIM808
#include <TinyGsmClient.h> // Biblioteca que configura el módem GSM

const char apn[] = "wap.tmovil.cl"; // Tu APN
const char user[] = "wap";
const char pass[] = "wap";

HardwareSerial Serial_SIM_Module(1);

TinyGsm sim808(Serial_SIM_Module);
TinyGsmClient client(sim808);

#else

WiFiClient client;
;

#endif

const char server[] = "mongo-arduino-cim.taicrosxy.workers.dev";
const char resource[] = "/api/ArduinoData";
const int port = 80;


#ifdef TIPO_AGUAS_GRISES
const String tipo = "AguasGris";
const int id = 1; // TODO CAMBIAR PARA CADA MAQUINA
#endif

#ifdef TIPO_AGUAS_LLUVIA
const String tipo = "AguasLluvia";
const int id = 2; // TODO CAMBIAR PARA CADA MAQUINA
#endif

#ifdef TIPO_ATRAPA_NIEBLA
const String tipo = "AtrapaNiebla";
const int id = 3; // TODO CAMBIAR PARA CADA MAQUINA
#endif

#ifdef TIPO_ENDURECIMIENTO_SUELO
const String tipo = "EndurecimientoSuelo";
const int id = 4; // TODO CAMBIAR PARA CADA MAQUINA
#endif

//const int id = 1; // TODO CAMBIAR PARA CADA MAQUINA

const String otaName = "Maqueta"+String(id)+tipo;
bool sendingHTTP = false;

// mongoEndpointAPIKEY =  hDYzA5V8btEmWF0tH1Pe1E6MVolfd5QSzaVCmJjOaOxcGl9WUNdrW0bB54mHn3m8

HttpClient http(client, server, port);

#ifdef USE_GSM

bool networkConnect()
{
    Serial.print("Esperando red...");
    if (!sim808.waitForNetwork())
    {
        Serial.println("Falló el modem");
        // return false;
    }

    if (sim808.isNetworkConnected())
    {
        Serial.println("----> Network connected");
    }

    Serial.print("Conectando a ");
    Serial.print(apn);

    if (!sim808.gprsConnect(apn, user, pass))
    {
        Serial.println("Falló el GPRS");
        return false;
    }

    if (sim808.isGprsConnected())
    {
        Serial.println("----> GPRS connected");
    }

    return true;
}

// Configura el módem GPRS
bool modemConfig()
{
    // Inicia el módem
    Serial.println("Configurando el módem...");
    if (!sim808.restart())
        return false;

    // Conecta a la red
    return networkConnect();
}

bool getGPSPos(float *latitud, float *longitud)
{
    float lat, lon;

    // Intenta obtener los valores de GPS
    if (sim808.getGPS(&lat, &lon))
    { // Obtiene la primera posicicon
        *latitud = lat;
        *longitud = lon;

        return true;
    }
    else
    {
        return false;
    }
}

#endif

String getArduinoDataJson(String data)
{
    return R"({ "arduinoData":  )" + data + R"(})";
}

String getStadisticsJson(String statFile)
{
    File file = SD.open(statFile.c_str(), FILE_READ);
    String data = "";

    // PUEDE FALLAR EL BUFFER
    if (file)
    {
        while (file.available())
        {
            data += file.readStringUntil('\n') + "\n";
        }
        file.close();
    }

    if (data.isEmpty())
    {
        return "";
    }

    Serial.println(data);

    String json = R"(
        {
            "type": "Stadistics",
            "id": )" +
                  String(id) + R"(,
            "data": [)";

    int index = 0;

    while (index < data.length())
    {
        int semicolonIndex = data.indexOf(";", index);
        int newlineIndex = data.indexOf("\n", index);

        if (semicolonIndex == -1 || newlineIndex == -1)
        {
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

String getPositionJson(float lat, float lon, int type)
{
    return R"(
        {
            "type": "Position",
            "id": )" +
           String(id) + R"(,
            "data": {
                "latitude": )" +
           String(lat, 12) + R"(,
                "longitude": )" +
           String(lon, 12) + R"(,
                "type": )" +
           String(type) + R"(,
                "date": ")" +
           getDateTime() + R"("
            }
        }
        
    )";
}

String getStatusJson(String status, int type)
{
    return R"(
        {
            "type": "Status",
            "id": )" +
           String(id) + R"(,
            "data": {
                "info": ")" +
           status + R"(",
                "type": )" +
           String(type) + R"(,
                "date": ")" +
           getDateTime() + R"("
            }
        }
        
    )";
}

int sendHttpQuery(String json)
{

    while (sendingHTTP)
    {
        delay(1000);
    }

    sendingHTTP = true;

    Serial.println("Intentando realizar conexion a la base de datos...");

    http.beginRequest();
    if (json == "")
    {
        http.get(resource);
        http.sendHeader("authorization", "pURzWbUHfRPJrOKoRTFnbTCrFAeBixKDmgjOJ85HqRDp47VWvSo1E2hsvZLjheCr");
    }
    else
    {
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
    if (status <= 0)
    {
        delay(1000);
        sendingHTTP = false;
        return 400;
    }

    Serial.println(F("Headers:"));
    while (http.headerAvailable())
    {
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
    sendingHTTP = false;
    Serial.println("Comunicacion con la base de datos exitosa");
    return status;
}
