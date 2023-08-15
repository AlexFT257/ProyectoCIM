#include <Arduino.h>

// #include "BluetoothSerial.h"
//  #include <DHT.h>
#include <HardwareSerial.h> // Comunicación UART con el módulo SIM808
#include <SPI.h>
#include <Wire.h>

#include "DataLogger.h"
#include "GPRS.h"     // Funciones de configuración del GPRS
#include "GPS.h"      //Funciones de GPS
#include "Sensores.h" // Funciones de configuración de los sensores

// pines de la maqueta
//#define HUMIDIFICADOR_PIN 13
#define BOMBA_PIN 14       //   7 en azul
#define RECUPERADOR_PIN 25 // 3

#define ROCIADOR_PIN 26

#define RX_PIN \
    17 // Pin RX serial que debe conectarse al TX del SIM808, se utiliza para
       // configurar la variable "mySerial" 
#define TX_PIN \
    16 // Pin TX serial que debe conectarse al RX del SIM808, se utiliza para
       // configurar la variable "mySerial"
const int BAUD_RATE = 9600;

int personasDetectadas = 0, botonesDetectados = 0;

TaskHandle_t HeavySensors;
TaskHandle_t FastSensors;
TaskHandle_t SlowSensors;

// GSM PIN
#define GSM_PIN "1234"

void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);

void Sequence();

void FastSensors_Task(void *);
void SlowSensors_Task(void *);
void HeavyTasks(void *);

void Ping();

void setup()
{
    Serial.begin(BAUD_RATE); // Establece la velocidad de baudios de la consola

    if (!setUpDataLogger())
    {
        delay(2000);
        ESP.restart();
    }
    else
    {
        /* File file = SD.open("/data.txt");
        if (!file)
        {
            Serial.println("El archivo no existe");
            Serial.println("Creando archivo. ..");
            writeFile(SD, "/data.txt", "Lectura de fecha, hora y contador");
        }
        else
        {
            Serial.println("El archivo ya existe");
        }
        file.close(); */
    }

    delay(1000);

    /* Serial_SIM_Module.begin(
        BAUD_RATE, SERIAL_8N1, RX_PIN,
        TX_PIN); // Configura la velocidad de baudios del ESP32 UART

    Serial.println(
        "Esperando..."); // Configura la velocidad de baudios utilizada por el
                         // monitor serial y el SIM808
    delay(3000);

    sim808.setBaud(BAUD_RATE); // Configura la velocidad de baudios utilizada por
                               // el monitor serial y el SIM808
    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && sim808.getSimStatus() != 3)
    {
        sim808.simUnlock(GSM_PIN);
    }

    Serial.println("GSM desbloqueada");

    // Configura e inicializa el GPRS
    if (!modemConfig())
    {
        Serial.println("Fallo en la inicialización del módem");
        delay(2000);
        //ESP.restart();
    }

    Serial_SIM_Module.println("AT+CGNSPWR=1");
    Serial.println("Calidad de la señal: " + String(sim808.getSignalQuality()));

    Serial.println("Activando GPS...");

    if (!sim808.enableGPS())
    { // Activa la función de GPS del módulo
        Serial.println("Fallo en la inicialización del GPS");
    }

    delay(40000);

    Serial.println("GPS Activado");
    Serial.println("Intentando obtener posicion de referencia");

    if (!getGPSPos(&LATITUDE_REFERENCE, &LONGITUDE_REFERENCE))
    {
        Serial.println("Fallo en la inicialización del GPS");
        delay(500);
        //ESP.restart();
    }
    else
    {
        Serial.println("Posicion inicial:");
        Serial.println("Latitud: " + String(LATITUDE_REFERENCE, 6));
        Serial.println("Longitud: " + String(LONGITUDE_REFERENCE, 6));
    }
 */
    // if(sendHttpQuery() != 200){
    //     Serial.println("Error en la conexion a la base de datos");
    // }

    // setup de los sensores
    FlowSensor();
    // DHTSensor();
    PIRSensor();

    // setup de los actuadores
    // pinMode(HUMIDIFICADOR_PIN, OUTPUT);
    pinMode(BOMBA_PIN, OUTPUT);
    pinMode(RECUPERADOR_PIN, OUTPUT);

    digitalWrite(RECUPERADOR_PIN, HIGH);
    digitalWrite(BOMBA_PIN, HIGH);
    // pinMode(ROCIADOR_PIN, OUTPUT);

    // Muestra en la consola
    Serial.println("Dispositivo configurado exitosamente");

    xTaskCreate(HeavyTasks, "HeavySensors", 4096, NULL, tskIDLE_PRIORITY, NULL);
    delay(3000);
    xTaskCreatePinnedToCore(FastSensors_Task, "FastSensors", 10000, NULL, 1, &FastSensors, 1);
    // xTaskCreatePinnedToCore(SlowSensors_Task, "SlowSensors", 10000, NULL, 1,
    // &SlowSensors, 1);
}

// funcion para fortmatear la fecha y hora en un formato legible por el DateTime
// de la libreria RTClib
// @param void
// @return String fecha y hora en formato legible
String dateNow()
{
    DateTime now = rtc.now();
    return String(now.year(), DEC) + "/" + String(now.month(), DEC) + "/" +
           String(now.day(), DEC) + "T" + String(now.hour(), DEC) + ":" +
           String(now.minute(), DEC) + ":" + String(now.second(), DEC);
}

/* void Ping() {
    Serial.println("Getting local IP address");
    Serial_SIM_Module.println("AT+CIFSR\r");
    delay(5000);
    Serial.println("Ping to Google");
    Serial_SIM_Module.println("AT+CIPPING=\"www.google.com\"\r");
    delay(5000);
    // Print the response on the Serial Monitor
    while (Serial_SIM_Module.available() > 0) {
        Serial.write(Serial_SIM_Module.read());
    }
}  */

void loop()
{
    /* double flow = getFlow();
    float t, h, lat, lon;

    getTemperatureAndHumidity(&t, &h);
    // getGPSPos(&lat, &lon);

    int pirValue = readPIR();

    delay(5000); */
}

void HeavyTasks(void *pvParameters)
{
    for (;;)
    {
        float lat, lon;
        String dist = "";
        getGPSPos(&lat, &lon);
        Serial.println("Latitud: " + String(lat) + " Longitud: " + String(lon));

        if (deviceIsTooFar(lat, lon, &dist))//
        {
            Serial.println("El dispositivo se esta alejando");
            for (;;)
            {

                String json = R"(
                    {
                        "arduinoData": {
                            "type": "Position",
                            "data": {
                                "latitude": )" +
                              String(lat) + R"(,
                                "longitude": )" +
                              String(lon) + R"(
                            }
                        }
                    }
                )";

                int res = sendHttpQuery(json); // MANDAR DATA

                if (res != 200)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // MANDAR INFORMACION NORMAL EN CASO DE ESTAR EN LA HORA
        }

        if (nextSchedule <= checkLastTime())
        {
            for (;;)
            {
                int res = 200; // sendHttpQuery(); // MANDAR DATA

                if (res != 200)
                {
                    setNextConection();
                    continue;
                }
                else
                {
                    break;
                }
                delay(1000);
            }
        }

        delay(10000);
    }
}

void FastSensors_Task(void *pvParameters)
{
    const int bombaDelay = 9, recuperadorDelay = 35;
    bool pirActivated = false;

    for (;;)
    {

        int pirValue = readPIR();
        int buttonValue = readButton();
        
        if (pirValue == 1)
        {
            if (!pirActivated)
            {
                personasDetectadas++;
                Serial.println("Personas detectadas: " + String(personasDetectadas));
                pirActivated = true;
            }
        }
        else
        {
            pirActivated = false;
        }

        Serial.println(buttonValue);
        if (buttonValue == 1)
        {
            botonesDetectados++;
            digitalWrite(BOMBA_PIN, LOW); // LOW porque funciona alrevez
            int i = 0;
            while(i++ < bombaDelay){
                delay(1000);
                double flow = getFlow();
                Serial.println("Flujo: " + String(flow));
        
            }
            i = 0;
            Serial.println("prendiendo bomba");
            digitalWrite(BOMBA_PIN, HIGH);
            digitalWrite(RECUPERADOR_PIN, LOW);
            Serial.println("prendiendo Recuperador");
             while(i++ <= recuperadorDelay){
                delay(1000);
                //double flow = getFlow();
                //Serial.println("Flujo: " + String(flow));
            }

            digitalWrite(RECUPERADOR_PIN, HIGH);
            delay(1000);
        }

        delay(500);
    }
}

void SlowSensors_Task(void *pvParameters)
{
    for (;;)
    {
        float t, h;
        getTemperatureAndHumidity(&t, &h);
        delay(5000);
    }
}


