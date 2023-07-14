#include <DHT.h>

unsigned long pulse_freq;  // Frequency of pulses (interrupt function)
unsigned long currentTime;
unsigned long lastTime;

#define FLOW_PIN 25   // Pin connected to the sensor
#define PIR_PIN 27
#define DHT_PIN 14

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

int pirState = LOW;
int val = 0;

void pulse() { pulse_freq++; }

/*
*   Funcion que configura el sensor de flujo de agua
* @return void
*/
void FlowSensor() {
    pinMode(FLOW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulse, RISING);
    currentTime = millis();
    lastTime = currentTime;
    Serial.println("Configuracion del sensor de flujo de agua exitosa");
}

/*
*   Funcion que configura el sensor de temperatura y humedad
* @return void
*/
void DHTSensor() {
    dht.begin();
    delay(5000);
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        Serial.println("Error en la lectura del sensor de temperatura y humedad");
    } else {
        Serial.println("Temperatura: " + String(t) + "°C");
        Serial.println("Humedad: " + String(h) + "%");
        Serial.println("Configuracion del sensor de temperatura y humedad exitosa");
    }
}

/*
*   Funcion que configura el sensor PIR
* @return void
*/
void PIRSensor() {
    pinMode(PIR_PIN, INPUT);
    Serial.println("Configuracion del sensor PIR exitosa");
}


/*
*   Funcion que retorna el flujo de agua en L/min
* @return double flujo de agua en L/min (-1 si falla)  
*/
double getFlow(){
    Serial.println("Midiendo Flujo");
    currentTime = millis();
    double flow = -1;
    // Every second, calculate and print L/Min
    if (currentTime >= (lastTime + 1000)) {
        lastTime = currentTime;
        // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
        double flow = (pulse_freq / 7.5);
        pulse_freq = 0;  // Reset Counter
        Serial.print(flow, DEC);
        Serial.println(" L/Min");
    }
    return flow;
}

/*
*   Funcion que retorna la temperatura y humedad
* @param float* temperatura
* @param float* humedad
* @return void
*/
void getTemperatureAndHumidity(float* temperature, float* humidity) {
    *temperature = dht.readTemperature();
    *humidity = dht.readHumidity();

    if (isnan(*temperature) || isnan(*humidity)) {
        Serial.println("Error en la lectura del sensor de temperatura y humedad");
    } else {
        Serial.println("Temperatura: " + String(*temperature) + "°C");
        Serial.println("Humedad: " + String(*humidity) + "%");
        // inserte codigo para guardar los datos en sd
    }
}

/*
*   Funcion que retorna el estado del sensor PIR
* @return int estado del sensor PIR (1 si está activado, 0 si está desactivado)
*/
int readPIR() {
    val = digitalRead(PIR_PIN);
    if (val == HIGH) {        // si está activado
        if (pirState == LOW)  // si previamente estaba apagado
        {
            Serial.println("Sensor activado");
            pirState = HIGH;
        }
    } else {                   // si esta desactivado
        if (pirState == HIGH)  // si previamente estaba encendido
        {
            Serial.println("Sensor parado");
            pirState = LOW;
        }
    }
    return val;
}