// #define TIPO_AGUAS_GRISES
//#define TIPO_AGUAS_LLUVIA
// #define TIPO_ATRAPA_NIEBLA
//#define TIPO_ATRAPA_NIEBLA
//#define TIPO_ENDURECIMIENTO_SUELO

#include <Arduino.h>

// #include "BluetoothSerial.h"
//  #include <DHT.h>
#include <HardwareSerial.h> // Comunicación UART con el módulo SIM808
#include <SPI.h>
#include <Wire.h>

#include "Sensores.h" // Funciones de configuración de los sensores

#ifdef TIPO_AGUAS_GRISES
#define tiempoBomba 9
#define tiempoRecuperador 23
#endif

#ifdef TIPO_AGUAS_LLUVIA
#define tiempoBomba 9
#endif

#ifdef TIPO_ATRAPA_NIEBLA
#define tiempoBomba 5
#endif

#ifdef TIPO_ENDURECIMIENTO_SUELO
#define tiempoBomba 9
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "DataLogger.h"
#include "GPRS.h"     // Funciones de configuración del GPRS
#include "GPS.h"      //Funciones de GPS

// pines de la maqueta
// #define HUMIDIFICADOR_PIN 13
#define BOMBA_PIN 25 //   7 en azul
// #define AUDIO_PIN 25       // 8    3 es 25
#define RECUPERADOR_PIN 12 // 8    3 es 25

#define ROCIADOR_PIN 26 //

#define RX_PIN \
    16 // Pin RX serial que debe conectarse al TX del SIM808, se utiliza para
       // configurar la variable "mySerial"
#define TX_PIN \
    17 // Pin TX serial que debe conectarse al RX del SIM808, se utiliza para
       // configurar la variable "mySerial"
const int BAUD_RATE = 9600;

int personasDetectadas = 0, botonesDetectados = 0, buttonValue = 0;

float flowProm; // Promedio del flow
String humedadTemp;

TaskHandle_t FastSensors;
TaskHandle_t SlowSensors;
bool connectedInternet = false, stillPlaying = false;
// GSM PIN
#define GSM_PIN "1234"

void FastSensors_Task(void *);
void SlowSensors_Task(void *);

void Ping();

/* void correr()
{
    Serial.println("AAAA");
    // Check if data is available from the SIM808 module
    while (Serial_SIM_Module.available())
    {
        // Read and process data from the module
        char incomingChar = Serial_SIM_Module.read();
        Serial.print(incomingChar);
        // Handle incoming data here
    }

    // Your main code goes here
} */

/* void sendATCommand(String command)
{
    Serial_SIM_Module.println(command);

    // Wait for and read the response
    delay(3000); // Adjust the delay as needed
    while (Serial_SIM_Module.available())
    {
        String response = Serial_SIM_Module.readStringUntil('\n');
        Serial.println(response);
        // Process the response as needed
    }
} */

/* void configureGPS()
{
    // Enable GPS
    sendATCommand("AT+CGPSPWR=1");

    // Configure GPS fix mode (e.g., set to auto mode)
    sendATCommand("AT+CGPSRST=0");

    // Set GPS NMEA data output format (e.g., RMC and GGA sentences)
    sendATCommand("AT+CGPSOUT=32");

    // Request GPS information
    sendATCommand("AT+CGPSINFO=1");
} */

/* void configat()
{
    // Start the serial communication with the specified BAUD_RATE
    // Serial_SIM_Module.begin(BAUD_RATE);

    // Initialize other necessary settings if needed
    // ...

    Serial.println("creg mode:");
    // Check network registration status
    sendATCommand("AT+CREG?");
    delay(3000);
    correr();

    Serial.println("attach the gprs:");
    // attach GPRS
    sendATCommand("AT+CGATT=1");
    delay(3000);
    correr();

    // Set the APN (Access Point Name)

    Serial.println("apn:");
    // Replace "APN," "username," and "password" with actual values
    sendATCommand("AT+CSTT=\"wap.tmovil.cl\",\"wap\",\"wap\"");
    delay(3000);
    correr();

    Serial.println("creg mode:");
    // Check network registration status
    sendATCommand("AT+CREG?");
    delay(3000);
    correr();

    // changing the operator manualiy
    Serial.println("operator:");
    sendATCommand("AT+COPS?");
    delay(3000);
    correr();


    Serial.println("Check gprs attachment:");
    // Check GPRS attachment status
    sendATCommand("AT+CGATT?");
    delay(3000);
    correr();


    //AT+COPS=1,0,"Network_Name"


    //delay(10000);

    Serial.println("ip:");
    // Check IP address after establishing the connection
    sendATCommand("AT+CIFSR");
    delay(3000);
    correr();
    Serial.println("signal streng:");
    // Check network signal strength
    sendATCommand("AT+CSQ");
    delay(3000);
    correr();
    // Configure the SIM808 module for GPS
    delay(3000);
    // configureGPS();
} */

void setupOTA(void *pvParameters)
{
#ifdef USE_GSM
    WiFi.mode(WIFI_AP);

#else // clave 2504KG.-
    WiFi.mode(WIFI_AP_STA);

    Serial.print("Attempting to connect to WPA SSID: ");
    // Connect to WPA/WPA2 network:

   // WiFi.begin("Ficom", "2504KG.-");
 WiFi.begin("wifi-ubb", "soporte-dci");
    Serial.println();
    Serial.print("Connecting");
    /* int max = 30; */
    while (WiFi.status() != WL_CONNECTED /* && max > 0 */)
    {
        //Serial.println(WiFi.localIP());
        /* max--; */
        delay(500);
        Serial.print(".");
    }


    connectedInternet = true;

#endif

    WiFi.softAP(otaName, "Cim12345");
    delay(1000);
    IPAddress IP = IPAddress(10, 10, 10, 1);
    IPAddress NMask = IPAddress(255, 255, 255, 0);
    WiFi.softAPConfig(IP, IP, NMask);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]()
                       { Serial.println("Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
     vTaskDelete(NULL);
}

void setup()
{
    Serial.begin(BAUD_RATE); // Establece la velocidad de baudios de la consola

    if (!setUpDataLogger())
    {
        delay(2000);

        ESP.restart();
    }

    delay(1000);
    /* setupOTA(); */
    xTaskCreate(setupOTA, "OTAUpdateTask", 4096, NULL, 1, NULL);

#ifdef USAR_GSM
    if (!sim808.init())
    {
        Serial.println("Error al inicializar el GSM");
    }

    Serial_SIM_Module.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN); // Configura la velocidad de baudios del ESP32 UART

    Serial.println("Esperando..."); // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808
    delay(3000);

    sim808.setBaud(BAUD_RATE); // Configura la velocidad de baudios utilizada por el monitor serial y el SIM808

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && sim808.getSimStatus() != 3)
    {
        sim808.simUnlock(GSM_PIN);
    }

    Serial.println("GSM desbloqueada");

    // configat();

    // Configura e inicializa el GPRS
    if (!modemConfig())
    {
        Serial.println("Fallo en la inicialización del módem");
        delay(2000);
        ESP.restart();
    }
    Serial_SIM_Module.println("AT+CGNSPWR=1");
    Serial.println("Calidad de la señal: " + String(sim808.getSignalQuality()));

    Serial.println("Activando GPS...");

    if (sendHttpQuery("") != 200)
    { // CHECK SERVER CONECTION
        Serial.println("Error en la conexion a la base de datos");
    }

    while (LATITUDE_REFERENCE == 0)
    {
        if (!sim808.enableGPS())
        { // Activa la función de GPS del módulo
            Serial.println("Fallo en la inicialización del GPS");
            delay(1000);
            continue;
        }

        delay(45000);

        Serial.println("GPS Activado");

        while (LATITUDE_REFERENCE == 0)
        {
            Serial.println("Intentando obtener posicion de referencia");

            if (!getGPSPos(&LATITUDE_REFERENCE, &LONGITUDE_REFERENCE))
            {
                Serial.println("Fallo en la inicialización del GPS");
                delay(500);
            }
            else
            {
                Serial.println("Posicion inicial:");
                Serial.println("Latitud: " + String(LATITUDE_REFERENCE, 12));
                Serial.println("Longitud: " + String(LONGITUDE_REFERENCE, 12));
            }
            delay(1000);
        }
    }
#endif

    // setup de los sensores
    FlowSensor();
    PIRSensor();
    
#ifdef TIPO_ATRAPA_NIEBLA
    DHTSensor();
#endif

    // setup de los actuadores
    pinMode(BOMBA_PIN, OUTPUT);
    digitalWrite(BOMBA_PIN, HIGH);

#ifdef TIPO_AGUAS_GRISES
    pinMode(RECUPERADOR_PIN, OUTPUT);
    digitalWrite(RECUPERADOR_PIN, HIGH);
#endif

    // Muestra en la consola
    Serial.println("Dispositivo configurado exitosamente");

    delay(3000);

    xTaskCreatePinnedToCore(FastSensors_Task, "FastSensors", 10000, NULL, 1, &FastSensors, 1);
    xTaskCreatePinnedToCore(SlowSensors_Task, "SlowSensors", 10000, NULL, 1, &SlowSensors, 1);
}

void loop()
{

    ArduinoOTA.handle();
   // Serial.println("BUTTON: " + String(buttonValue));
    if (buttonValue == 1)
    {
        Serial.println("encendiendo audio");
        digitalWrite(BOMBA_PIN, LOW);
        delay(100);
        digitalWrite(BOMBA_PIN, HIGH);

        #ifdef TIPO_AGUAS_LLUVIA
        delay(32000); // el audio dura 32 segundos
        #endif

        #ifdef TIPO_AGUAS_GRISES
        delay(40000); // el audio dura 40 segundos
        #endif

        #ifdef TIPO_ATRAPA_NIEBLA
        delay(32000); // el audio dura 32 segundos
        #endif

        #ifdef TIPO_ENDURECIMIENTO_SUELO
        delay(20000); // DEPNDE DE CUANTO DURA EL AUDIO
        #endif


        Serial.println("encendiendo audio");
        digitalWrite(BOMBA_PIN, LOW);
        delay(100);
        digitalWrite(BOMBA_PIN, HIGH);
        stillPlaying = false;
        //buttonValue = 0;
    }

    delay(100);
}

void SlowSensors_Task(void *pvParameters)
{
    String toSave;
    for (;;)
    {
        
#ifdef USAR_GSM
        float lat, lon;
        String dist = "";
        getGPSPos(&lat, &lon);

        Serial.println("Latitud: " + String(lat, 12) + " Longitud: " + String(lon, 12));

        // Serial.println("Dato: " + toSave);

        if (deviceIsTooFar(lat, lon, &dist))
        {
            Serial.println("El dispositivo se esta alejando");

            for (;;)
            {
                for (;;)
                {
                    int res = sendHttpQuery(getArduinoDataJson(getPositionJson(lat, lon, 1))); // MANDAR DATA

                    if (res != 200)
                    {
                        Serial.println("fallo al mandar la posicion");
                        delay(1000);
                        continue;
                    }
                    else
                    {
                        Serial.println("Posicion enviada");
                        break;
                    }
                }

                getGPSPos(&lat, &lon);
                delay(5000);
            }
        }
#endif

        // Serial.println("nextSchedule: "+ getDateTime(nextSchedule));
        if (nextSchedule <= checkLastTime())
        {
            for (;;)
            {
                String stats = getStadisticsJson(statFile);
                if (stats != "")
                {
                    stats += ",";
                }
#ifdef USE_GSM
                String toSend = "[" + getPositionJson(lat, lon, 0) + "," + stats + getStatusJson("El flujo es: " + String(flowProm), flowProm > 0.2f ? 0 : 1) + "]";
#else       

    #ifdef TIPO_ATRAPA_NIEBLA
                String toSend = "[" + stats + getStatusJson("El flujo es: " + String(flowProm)  + " \nLa humedad y temperatura es: " + String(humedadTemp), flowProm > 0.2f ? 0 : 1) + "]";
    #else
                String toSend = "[" + stats + getStatusJson("El flujo es: " + String(flowProm) , flowProm > 0.2f ? 0 : 1) + "]";
    #endif

#endif

                String json = getArduinoDataJson(toSend);
                Serial.println(json);

                int res = sendHttpQuery(json); // MANDAR DATA

                if (res != 200)
                {
                    Serial.println("Fallo en la conexion no se pudo enviar");
                    continue;
                }
                else
                {
                    Serial.println("Fallo en la conexion enviado");
                    setNextConection(12);
                    break;
                }
                delay(1000);
            }

            // CREAR ARCHIVO NUEVO DIA
            statFile = "/Stadistics/" + getDateToFile() + ".txt";
            File file = SD.open(statFile.c_str(), FILE_WRITE);
            file.close();
        }
#ifdef TEST_MODE
        delay(1000 * 60);
#elif USE_GSM
        delay(1000 * 60 * 5); // Verificar posicion cada 5 minutos
#else
         delay(1000 * 60 * 60); // Verificar posicion cada 5 minutos
#endif

    }
}

void FastSensors_Task(void *pvParameters)
{

    bool pirActivated = false;

    // probar
    // statFile = "/Stadistics/" + getDateToFile() + ".txt";
    String toSave;

    for (;;)
    {
        int pirValue = readPIR();
        buttonValue = readButton() && !stillPlaying;
        // float temp = dht.readTemperature();

        // Serial.println("temperatura: " + String(temp));

        if (pirValue == 1)
        {
            if (!pirActivated)
            {
                personasDetectadas++;
                Serial.println("Personas detectadas: " + String(personasDetectadas));
                pirActivated = true;
                // se guarda la estadistica (deteccion=0)
                toSave = getDateTime() + ";0" + "\n";
                appendFile(SD, statFile.c_str(), toSave.c_str());
            }
        }
        else
        {
            pirActivated = false;
        }

        if (buttonValue == 1)
        {
            stillPlaying = true;
            delay(2000);
            botonesDetectados++;
            // se guarda la estadistica del boton pulsado
            toSave = getDateTime() + ";1" + "\n";
            appendFile(SD, statFile.c_str(), toSave.c_str());

            Serial.println("Boton Apretado - Prendiendo BOMBA");
            digitalWrite(BOMBA_PIN, LOW); // LOW porque funciona alrevez

            double flowSum = 0;
            int i = 0;
            while (i++ < tiempoBomba)
            {
                delay(1000);
                flowSum += getFlow();
                Serial.println("Flujo promedio: " + String(flowSum / i));
            }



            flowSum /= i;
            flowProm = (flowProm + flowSum) / 2;
            #ifdef TIPO_ATRAPA_NIEBLA
            float temp, hum;
            getTemperatureAndHumidity(&temp, &hum);
            
            humedadTemp = String(temp)+"°" + String(hum) + "%";
            #endif
            digitalWrite(BOMBA_PIN, HIGH);
            buttonValue = 0;

            if (flowSum < 0.2 && connectedInternet)
            {
                 for (;;)
                 {
                     String json = getArduinoDataJson(getStatusJson("Perdida de flujo: " + String(flowProm), 1));
                     Serial.println(json);

                     int res = sendHttpQuery(json); // MANDAR DATA

                     if (res != 200)
                     {
                         Serial.println("Fallo en la bomba no se pudo enviar");
                         continue;
                     }
                     else
                     {
                         Serial.println("Fallo en la bomba enviado");
                         break;
                     }

                     delay(2000);
                 }
            }

#ifdef TIPO_AGUAS_GRISES
            i = 0;

            digitalWrite(RECUPERADOR_PIN, LOW);
            Serial.println("Apagando BOMBA y prendiendo RECUPERADOR");

            while (i++ <= tiempoRecuperador)
            {
                delay(1000);
            }
            delay(100);
            digitalWrite(RECUPERADOR_PIN, HIGH);
            Serial.println("Apagando RECUPERADOR");
#endif
            delay(1000);
        }

        delay(500);
    }
}
