#include <Arduino.h>

// #include "BluetoothSerial.h"
//  #include <DHT.h>
#include <HardwareSerial.h>  // Comunicación UART con el módulo SIM808
#include <SPI.h>
#include <Wire.h>

#include "DataLogger.h"
#include "GPRS.h"      // Funciones de configuración del GPRS
#include "GPS.h"       //Funciones de GPS
#include "Sensores.h"  // Funciones de configuración de los sensores

// pines de la maqueta
// #define HUMIDIFICADOR_PIN 13
#define BOMBA_PIN 14        //   7 en azul
#define RECUPERADOR_PIN 25  // 3

#define ROCIADOR_PIN 26  //

#define RX_PIN \
    16  // Pin RX serial que debe conectarse al TX del SIM808, se utiliza para
        // configurar la variable "mySerial"
#define TX_PIN \
    17  // Pin TX serial que debe conectarse al RX del SIM808, se utiliza para
        // configurar la variable "mySerial"
const int BAUD_RATE = 9600;

int personasDetectadas = 0, botonesDetectados = 0;

float flowProm;  // Promedio del flow

TaskHandle_t HeavySensors;
TaskHandle_t FastSensors;
TaskHandle_t SlowSensors;

// GSM PIN
#define GSM_PIN "1111"

void writeFile(fs::FS& fs, const char* path, const char* message);
void appendFile(fs::FS& fs, const char* path, const char* message);

void FastSensors_Task(void*);
// void SlowSensors_Task(void*);
void HeavyTasks(void*);

void Ping();

void setup() {
    Serial.begin(BAUD_RATE);  // Establece la velocidad de baudios de la consola

    if (!setUpDataLogger()) {
        delay(2000);
        ESP.restart();
    }

    delay(1000);

    Serial_SIM_Module.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);  // Configura la velocidad de baudios del ESP32 UART

    Serial.println("Esperando...");  // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808
    delay(3000);

    sim808.setBaud(BAUD_RATE);  // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && sim808.getSimStatus() != 3) {
        sim808.simUnlock(GSM_PIN);
    }

    Serial.println("GSM desbloqueada");

    // Configura e inicializa el GPRS
    if (!modemConfig()) {
        Serial.println("Fallo en la inicialización del módem");
        delay(2000);
        ESP.restart();
    }
    Serial_SIM_Module.println("AT+CGNSPWR=1");
    Serial.println("Calidad de la señal: " + String(sim808.getSignalQuality()));

    Serial.println("Activando GPS...");


    while (LATITUDE_REFERENCE == 0) {
        if (!sim808.enableGPS()) {  // Activa la función de GPS del módulo
            Serial.println("Fallo en la inicialización del GPS");
            delay(1000);
            continue;
        }

        delay(15000);

        Serial.println("GPS Activado");
        
        while (LATITUDE_REFERENCE == 0) { 
            Serial.println("Intentando obtener posicion de referencia");

            if (!getGPSPos(&LATITUDE_REFERENCE, &LONGITUDE_REFERENCE)) {
                Serial.println("Fallo en la inicialización del GPS");
                delay(500);
            } else {
                Serial.println("Posicion inicial:");
                Serial.println("Latitud: " + String(LATITUDE_REFERENCE, 12));
                Serial.println("Longitud: " + String(LONGITUDE_REFERENCE, 12));
            }
            delay(1000);
        }
    }

    if (sendHttpQuery("") != 200) {  // CHECK SERVER CONECTION
        Serial.println("Error en la conexion a la base de datos"); 
    }

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
    /*  xTaskCreatePinnedToCore(SlowSensors_Task, "SlowSensors", 10000, NULL, 1,
     &SlowSensors, 1); */
}


void loop() {
    delay(1000);
}

void HeavyTasks(void* pvParameters) {
    String toSave;
    for (;;) {
        float lat, lon;
        String dist = "";
        getGPSPos(&lat, &lon);
        Serial.println("Latitud: " + String(lat) + " Longitud: " + String(lon));

        // Serial.println("Dato: " + toSave);

        if (deviceIsTooFar(lat, lon, &dist)) {
            Serial.println("El dispositivo se esta alejando");

            for (;;) {
                for (;;) {
                    int res = sendHttpQuery(getArduinoDataJson(getPositionJson(lat, lon, 1)));  // MANDAR DATA

                    if (res != 200) {
                        delay(1000);
                        continue;
                    } else {
                        break;
                    }
                    
                }

                getGPSPos(&lat, &lon);
                delay(5000);
            }
        }

        if (nextSchedule <= checkLastTime()) {
            for (;;) {
                String stats = getStadisticsJson(statFile);
                if(stats != ""){
                    stats += ",";
                }

                String json = getArduinoDataJson("[" + getPositionJson(lat, lon, 0) + "," + stats + getStatusJson(String(flowProm), 0) + "]");
                Serial.println(json);

                int res = sendHttpQuery(json);  // MANDAR DATA

                if (res != 200) {
                    continue;
                } else {
                    setNextConection(12);
                    break;
                }
                delay(1000);
            }

            // CREAR ARCHIVO NUEVO DIA
            statFile = "/Stadistics/" + getDateToFile() + ".txt";
            File file = SD.open(statFile.c_str(), FILE_APPEND);
        }

        delay(1000);
        // delay(1000 * 60 * 5); //Verificar posicion cada 5 minutos
    }
}

void FastSensors_Task(void* pvParameters) {
    const int bombaDelay = 9, recuperadorDelay = 35;
    bool pirActivated = false;

    // probar
    // statFile = "/Stadistics/" + getDateToFile() + ".txt";
    String toSave;

    for (;;) {
        int pirValue = readPIR();
        int buttonValue = readButton();

        if (pirValue == 1) {
            if (!pirActivated) {
                personasDetectadas++;
                Serial.println("Personas detectadas: " + String(personasDetectadas));
                pirActivated = true;
                // se guarda la estadistica (deteccion=0)
                toSave = getDateTime() + ";0" + "\n";
                appendFile(SD, statFile.c_str(), toSave.c_str());
            }
        } else {
            pirActivated = false;
        }

        if (buttonValue == 1) {
            botonesDetectados++;

            // se guarda la estadistica del boton pulsado
            toSave = getDateTime() + ";1" + "\n";
            appendFile(SD, statFile.c_str(), toSave.c_str());

            Serial.println("Boton Apretado - Prendiendo BOMBA");
            digitalWrite(BOMBA_PIN, LOW);  // LOW porque funciona alrevez

            double flowSum = 0;
            int i = 0;
            while (i++ < bombaDelay) {
                delay(1000);
                flowSum += getFlow();
                Serial.println("Flujo promedio: " + String(flowSum / i));
            }
            
            flowSum /= i;
            flowProm = (flowProm + flowSum) / 2;
            digitalWrite(BOMBA_PIN, HIGH);

            if (flowSum < 0.2) {
                for (;;) {
                    String json = getArduinoDataJson(getStatusJson("Perdida de flujo: " + String(flowProm), 1));
                    Serial.println(json);

                    int res = sendHttpQuery(json);  // MANDAR DATA

                    if (res != 200) {
                        continue;
                    } else {
                        break;
                    }

                    delay(2000);
                }
            }

            i = 0;


            digitalWrite(RECUPERADOR_PIN, LOW);
            Serial.println("Apagando BOMBA y prendiendo RECUPERADOR");

            while (i++ <= recuperadorDelay) {
                delay(1000);
            }

            digitalWrite(RECUPERADOR_PIN, HIGH);
            Serial.println("Apagando RECUPERADOR");
            delay(1000);
        }

        delay(500);
    }
}
