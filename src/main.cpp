#include <Arduino.h>
#include <HardwareSerial.h> // Comunicación UART con el módulo SIM808
#include <SPI.h>
#include <Wire.h>
#include<DHT.h>

#include "GPRS_Functions.h" // Funciones de configuración del GPRS

const double MAX_DISTANCE = 10; // Distancia límite utilizada para activar el envío de SMS de alerta de distancia, medida en metros

double LATITUDE_REFERENCE;
double LONGITUDE_REFERENCE;

double flow; //Liters of passing water volume
unsigned long pulse_freq; //Frequency of pulses (interrupt function)
const int FLOW_PIN = 25; //Pin connected to the sensor
unsigned long currentTime;
unsigned long lastTime;

#define DHT_PIN 14
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

#define PIR_PIN 27
int pirState = LOW;
int val = 0;

// set GSM PIN, if any
#define GSM_PIN "1234"

const int RX_PIN = 17; // Pin RX serial que debe conectarse al TX del SIM808, se utiliza para configurar la variable "mySerial"
const int TX_PIN = 16; // Pin TX serial que debe conectarse al RX del SIM808, se utiliza para configurar la variable "mySerial"

// const int pinLed = 17;  // Pin de señalización que se activa al mismo tiempo que se envía el SMS al celular
const int BAUD_RATE = 9600;

void serialConfig();
int sendHttpQuery();
bool smsConfig();
bool gpsConfig();
bool baudConfig();
bool deviceIsTooFar(float lat, float lon, String *distance);
void ping();
void simpleHttp();
void readFlow();
void pulse();

void setup()
{
    // pinMode(pinLed, OUTPUT);

    serialConfig();

    Serial.println("Esperando..."); // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808
    delay(3000);

    // if (!baudConfig())  // Informa al SIM808 la velocidad de baudios utilizada, se recomienda ejecutar este programa por primera vez a una velocidad de 9600
    //     ESP.restart();

    sim808.setBaud(BAUD_RATE); // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808
    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && sim808.getSimStatus() != 3)
    {
        sim808.simUnlock(GSM_PIN);
    }
    
    Serial.println("Configuración del SMS correcta");

    // Configura e inicializa el GPRS
    if (!modemConfig())
    {
        // Muestra en la consola
        Serial.println("Fallo en la inicialización del módem");
        delay(2000);
        ESP.restart();
    }

    // Activa la función de GPS del módulo
    // sim808.enableGPS();
    Serial_SIM_Module.println("AT+CGNSPWR=1");

    Serial.println("Configuración del GPS correcta, configurando módem...");
    Serial.println("Intentando obtener posicion de referencia");
    Serial.println("Signal quality: " + String(sim808.getSignalQuality()));
    
    delay(40000);
    Serial.println("Posicion de referencia obtenida: "+ sim808.getGPSraw());
    // Serial.println("Latitud: " + String(sim808.GPSdata.lat, 6));

    float lat, lon;
    // Intenta obtener los valores de GPS
    if(sim808.getGPS(&lat, &lon))  {  // Obtiene la primera posicicon
        LATITUDE_REFERENCE = lat;
        LONGITUDE_REFERENCE = lon;

        Serial.println("Posicion inicial:");
        Serial.println("Latitud: " + String(LATITUDE_REFERENCE, 6));
        Serial.println("Longitud: " + String(LONGITUDE_REFERENCE, 6));

    } else {
        Serial.println("Error en el GPS");
    }

    Serial.println("Latitud: " + String(lat, 6));
    Serial.println("Longitud: " + String(lon, 6)); 

    Serial.println("Intentando realizar conexion a la base de datos");
    // if(sendHttpQuery() != 200){
    //     Serial.println("Error en la conexion a la base de datos");
    // }

    Serial.println("Conexion a la base de datos exitosa");

    // configuracion del sensor de flujo de agua
    pinMode(FLOW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulse, RISING);
    currentTime = millis();
    lastTime = currentTime;

    Serial.println("Configuracion del sensor de flujo de agua exitosa");

    // configuracion del sensor de temperatura y humedad
    dht.begin();
    delay(5000);

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if(isnan(t) || isnan(h)){
        Serial.println("Error en la lectura del sensor de temperatura y humedad");
    }else{
        Serial.println("Temperatura: " + String(t) + "°C");
        Serial.println("Humedad: " + String(h) + "%");
        Serial.println("Configuracion del sensor de temperatura y humedad exitosa");
    }

    // configuracion del sensor PIR
    pinMode(PIR_PIN, INPUT);

    // Muestra en la consola
    Serial.println("Módem listo");
}

void pulse(){
    pulse_freq++;
}

void readFlow(){
    flow = .00225 * pulse_freq;
    Serial.print("Flow: ");
    Serial.print(flow,DEC);
    Serial.println(" L");
    // delay(500); // 500ms entre cada lectura
}

void readTemperatureAndHumidity(){
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if(isnan(t) || isnan(h)){
        Serial.println("Error en la lectura del sensor de temperatura y humedad");
    }else{
        Serial.println("Temperatura: " + String(t) + "°C");
        Serial.println("Humedad: " + String(h) + "%");
        // inserte codigo para guardar los datos en sd 
    }
}

void readPIR(){
    val = digitalRead(PIR_PIN);
    if (val == HIGH){   //si está activado 
        if (pirState == LOW)  //si previamente estaba apagado
        {
        Serial.println("Sensor activado");
        pirState = HIGH;
        }
    } else {   //si esta desactivado
        if (pirState == HIGH)  //si previamente estaba encendido
        {
        Serial.println("Sensor parado");
        pirState = LOW;
        }
    }
}

int sendHttpQuery()
{
    int statusCode = 500;

    // int err = 0;
    http.beginRequest();
    

    Serial.print(F("Performing HTTP GET request... "));
    http.get(resource);
    http.sendHeader("authorization", "pURzWbUHfRPJrOKoRTFnbTCrFAeBixKDmgjOJ85HqRDp47VWvSo1E2hsvZLjheCr");
    http.endRequest();
    // if (err != 0)
    // {
    //     Serial.println(F("failed to connect"));
    //     delay(10000);
    //     return 400;
    // }

    int status = http.responseStatusCode();
    Serial.print(F("Response status code: "));
    Serial.println(status);
    if (!status)
    {
        delay(10000);
        return 400;
    }

    Serial.println(F("Response Headers:"));
    while (http.headerAvailable())
    {
        String headerName = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        Serial.println("    " + headerName + " : " + headerValue);
    }

    int length = http.contentLength();
    if (length >= 0)
    {
        Serial.print(F("Content length is: "));
        Serial.println(length);
    }
    if (http.isResponseChunked())
    {
        Serial.println(F("The response is chunked"));
    }

    String body = http.responseBody();
    Serial.println(F("Response:"));
    Serial.println(body);

    Serial.print(F("Body length is: "));
    Serial.println(body.length());

    // Shutdown

    http.stop();
    Serial.println(F("Server disconnected"));

    // Close the connection
    client.stop();

    return status;
}

void serialConfig()
{
    Serial.begin(BAUD_RATE); // Establece la velocidad de baudios de la consola

    // Configura la velocidad de baudios del ESP32 UART
    Serial_SIM_Module.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN); // Esp32 lora 0 = RX, 22 = TX
}

/* Configuración de mensajes SMS*/
bool smsConfig()
{
    if (sendAT("AT+CMGF=1").indexOf("OK") < 0) // Establece el modo SMS como texto (0 = modo PDU, 1 = modo texto)
        return false;

    return true; // Si tiene éxito, devuelve true
}

/* Configuración relacionada con el GPS */
bool gpsConfig()
{
    if (sendAT("AT+CGNSPWR=1").indexOf("OK") >= 0)     // Enciende la fuente de energía del GNSS (1 = encendido, 0 = apagado)
        if (sendAT("AT+CGPSINF=0").indexOf("OK") >= 0) // Intenta obtener la ubicación GPS por primera vez
            return true;

    return false;
}

/* Informa al SIM808 sobre la velocidad de baudios utilizada, se recomienda ejecutar este programa por primera vez a una velocidad de 9600 */
bool baudConfig()
{
    if (sendAT("AT+IPR=" + String(9600)).indexOf("OK") >= 0) // Si tiene éxito, devuelve true //CAMBIADO
        return true;

    return false;
}

void loop()
{
    String distance, la, lo;
    float lat, lon;

    // // Intenta obtener los valores de GPS
    // if(sim808.getGPS(&lat, &lon))  {  // Obtiene la primera posicicon
    //     LATITUDE_REFERENCE = lat;
    //     LONGITUDE_REFERENCE = lon;

    //     Serial.println("Posicion inicial:");
    //     Serial.println("Latitud: " + String(LATITUDE_REFERENCE, 6));
    //     Serial.println("Longitud: " + String(LONGITUDE_REFERENCE, 6));

    // } else {
    //     Serial.println("Error en el GPS");
    // }

    Serial.println("Midiendo Flujo");
    currentTime = millis();
   // Every second, calculate and print L/Min
   if(currentTime >= (lastTime + 1000))
   {
      lastTime = currentTime; 
      // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
      flow = (pulse_freq / 7.5); 
      pulse_freq = 0; // Reset Counter
      Serial.print(flow, DEC); 
      Serial.println(" L/Min");
      
   }

   // para medir la cantidad de agua que se ha consumido
//    flow = .00225 * pulse_freq;
//    Serial.print(flow, DEC);
//    Serial.println("L");
//    delay(500);

    readTemperatureAndHumidity();

    readPIR();


    delay(5000);
}

/* Calcula la distancia a la que se encuentra el dispositivo, teniendo en cuenta la curvatura de la Tierra (valor devuelto en metros)*/
double getDistance(float lat, float lon)
{
    double dist = 60 * ((acos(sin(LATITUDE_REFERENCE * (PI / 180)) * sin(lat * (PI / 180)) + cos(LATITUDE_REFERENCE * (PI / 180)) * cos(lat * (PI / 180)) * cos(abs((lon - LONGITUDE_REFERENCE)) * (PI / 180)))) * (180 / PI));
    return dist * 1852;
}

/* Verifica si el dispositivo ha superado el límite de distancia*/
bool deviceIsTooFar(float lat, float lon, String *distance)
{
    double dist = getDistance(lat, lon);

    *distance = String(dist);

    if (dist > MAX_DISTANCE)
        return true;

    return false;
}
