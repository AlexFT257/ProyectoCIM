#include <RTClib.h>
#include <SD.h>

#include "FS.h"

#define SD_CS 5  // Se define el CS en el pin 5 y se nombra como SD_CS

RTC_DS1307 rtc;
DateTime lastValidDate;
DateTime nextSchedule;

/*  Escribir en la tarjeta SD
 @param fs tarjeta SD
 @param path ruta del archivo
 @param message mensaje a escribir
 @return void
 */

void writeFile(fs::FS& fs, const char* path, const char* message) {
    Serial.printf("Escribiendo archivo: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("No se pudo abrir el archivo para escribir");
        return;
    }
    if (file.print(message)) {
        Serial.println("Archivo escrito");
    } else {
        Serial.println("Escritura fallida");
    }
    file.close();
}

/*
Anexar datos a la tarjeta SD
 @param fs tarjeta SD
 @param path ruta del archivo
 @param message mensaje a escribir
 @return void
 */
void appendFile(fs::FS& fs, const char* path, const char* message) {
    // Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Error al abrir el archivo para adjuntar");
        return;
    }
    if (file.print(message)) {
        // Serial.println("Mensaje adjunto");
    } else {
        Serial.println("Adjuntar fallo");
    }
    file.close();
}

// funcion para guardar datos en el archivo especificado en la tarjeta SD
// @param file nombre del archivo
// @param toSave datos a guardar
// @return void
void saveToSDCard(String file, String toSave) {
    appendFile(SD, ("/" + file).c_str(), toSave.c_str());
}

void setNextConection() {
    if (nextSchedule.hour() == 12) {
        nextSchedule = DateTime(nextSchedule.year(), nextSchedule.month(), nextSchedule.day(), 23, 59, 0);
    } else {
        nextSchedule = DateTime(nextSchedule.year(), nextSchedule.month(), nextSchedule.day() + 1, 12, 0, 0);
    }
}

bool setUpDataLogger() {
    if (!rtc.begin()) {
        Serial.println("No se pudo encontrar el RTC");
        Serial.flush();
        return false;
    }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    lastValidDate = DateTime(F(__DATE__), F(__TIME__));

    if (!rtc.isrunning()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        lastValidDate = DateTime(F(__DATE__), F(__TIME__));
    } else {
        lastValidDate = rtc.now();
    }

    if (lastValidDate.hour() < 12) {
        nextSchedule = DateTime(lastValidDate.year(), lastValidDate.month(), lastValidDate.day(), 12, 0, 0);
    } else {
        nextSchedule = DateTime(lastValidDate.year(), lastValidDate.month(), lastValidDate.day(), 22, 40, 0);
    }

    // Inicializar tarjeta SD
    /* SD.begin(SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("Falló el montaje de la tarjeta");
        return false;
    }

    // almacena el tipo de sd
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("Sin tarjeta SD adjunta");
        return false;
    }

    Serial.println("Inicializando tarjeta SD...");

    if (!SD.begin(SD_CS)) {
        Serial.println("ERROR - ¡Falló la inicialización de la tarjeta 5D!");
        return false;
    }
    Serial.println("Tarjeta SD configurada"); */

    return true;
}

// funcion para obtener la fecha y hora actual en formato String
// @return String fecha y hora en formato legible
String getDateTime() {
    DateTime now = rtc.now();
    return String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
}

// funcion para obtener la fecha y hora del datetime en formato String
// @param date fecha y hora a convertir
// @return String fecha y hora en formato legible
String getDateTime(DateTime date) {
    return String(date.year()) + "-" + String(date.month()) + "-" + String(date.day()) + " " + String(date.hour()) + ":" + String(date.minute()) + ":" + String(date.second());
}

// funcion para obtener la fecha y hora actual (usar antes de usar cualquier otra funcion de fecha y hora)
// @return void
DateTime checkLastTime() {
    DateTime now = rtc.now();
    uint32_t diff = now.unixtime() - lastValidDate.unixtime();
    if (diff >= 0 && diff <= 600 + 60) {
        lastValidDate = now;
    } else {
        Serial.println("Diferencia: " + String(diff));
        Serial.println("Fecha actual: " + getDateTime(now));
        Serial.println("Fecha valida: " + getDateTime(lastValidDate));
        rtc.adjust(lastValidDate);
        Serial.println("Fecha ajustada: " + getDateTime(lastValidDate));
    }
    return lastValidDate;
}