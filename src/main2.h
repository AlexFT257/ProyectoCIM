

// Select your modem:
// #define TINY_GSM_MODEM_SIM800
 #define TINY_GSM_MODEM_SIM808
// #define TINY_GSM_MODEM_SIM868
// #define TINY_GSM_MODEM_SIM7000SSL
// #define TINY_GSM_MODEM_SIM7080
// #define TINY_GSM_MODEM_UBLOX
// #define TINY_GSM_MODEM_SARAR4
// #define TINY_GSM_MODEM_ESP8266
// #define TINY_GSM_MODEM_XBEE
// #define TINY_GSM_MODEM_SEQUANS_MONARCH

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
//#ifndef __AVR_ATmega328P__
#define SerialAT Serial1

// or Software Serial on Uno, Nano
/* #else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX //ALOMEJOR ES ESTE
#endif */

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// #define LOGGING  // <- Logging is for the HTTP library

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// set GSM PIN, if any
#define GSM_PIN "1234"

const int RX_PIN = 17;   // Pin RX serial que debe conectarse al TX del SIM808, se utiliza para configurar la variable "mySerial"
const int TX_PIN = 16;  // Pin TX serial que debe conectarse al RX del SIM808, se utiliza para configurar la variable "mySerial"


// flag to force SSL client authentication, if needed
// #define TINY_GSM_SSL_CLIENT_AUTHENTICATION

// Your GPRS credentials, if any

const char apn[] = "wap.tmovil.cl";  // Tu APN
const char user[] = "web";
const char pass[] = "web";

// Server details
/* const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int port = 443; */

#include <ArduinoHttpClient.h>
#include <TinyGsmClient.h>
#include<Wire.h>
#include <SPI.h>

TinyGsm modem(SerialAT);

TinyGsmClientSecure client(modem);
HttpClient http(client, "https://sa-east-1.aws.data.mongodb-api.com");

void setup() {
    // Set console baud rate
    SerialMon.begin(9600);
    delay(10);

    // !!!!!!!!!!!
    // Set your reset, enable, power pins here
    // !!!!!!!!!!!

    SerialMon.println("Wait...");

    // Set GSM module baud rate
    // TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX); 
    modem.setBaud(9600);
    // SerialAT.begin(9600);
    SerialAT.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);  // Esp32 lora 0 = RX, 22 = TX
    delay(6000);
    SerialAT.println("AT");

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // modem.init();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    } 
}

void loop() {

    // The XBee must run the gprsConnect function BEFORE waiting for network!
    // Serial.println("Connecting to GPRS...");
    // modem.gprsConnect(apn, user, pass);

    if(SerialAT.available()){
        SerialMon.write(SerialAT.read());
    }

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
    }

    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) {
        SerialMon.println("GPRS connected");
    }

    SerialMon.print(F("Performing HTTPS GET request... "));
    http.connectionKeepAlive();  // Currently, this is needed for HTTPS

    return;

    
    /* int err = http.get (resource);
    if (err != 0) {
        SerialMon.println(F("failed to connect"));
        delay(10000);
        return;
    } */

    int status = http.responseStatusCode();
    SerialMon.print(F("Response status code: "));
    SerialMon.println(status);
    if (!status) {
        delay(10000);
        return;
    }

    SerialMon.println(F("Response Headers:"));
    while (http.headerAvailable()) {
        String headerName = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        SerialMon.println("    " + headerName + " : " + headerValue);
    }

    int length = http.contentLength();
    if (length >= 0) {
        SerialMon.print(F("Content length is: "));
        SerialMon.println(length);
    }
    if (http.isResponseChunked()) {
        SerialMon.println(F("The response is chunked"));
    }

    String body = http.responseBody();
    SerialMon.println(F("Response:"));
    SerialMon.println(body);

    SerialMon.print(F("Body length is: "));
    SerialMon.println(body.length());

    // Shutdown

    http.stop();
    SerialMon.println(F("Server disconnected"));

    modem.gprsDisconnect();
    SerialMon.println(F("GPRS disconnected"));

    // Do nothing forevermore
    while (true) {
        delay(1000);
    }
}