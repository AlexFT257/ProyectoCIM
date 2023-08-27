#include <Arduino.h>

const float MAX_DISTANCE = 10;  // Distancia límite utilizada para activar el envío de alerta de distancia, medida en metros

float LATITUDE_REFERENCE = 0;
float LONGITUDE_REFERENCE = 0;


double getDistance(float lat, float lon) {
    double dist = 60 * ((acos(sin(LATITUDE_REFERENCE * (PI / 180)) * sin(lat * (PI / 180)) +
                              cos(LATITUDE_REFERENCE * (PI / 180)) * cos(lat * (PI / 180)) *
                                  cos(abs((lon - LONGITUDE_REFERENCE)) * (PI / 180)))) *
                        (180 / PI));
    return dist * 1852;
}

/* Verifica si el dispositivo ha superado el límite de distancia*/
bool deviceIsTooFar(float lat, float lon, String* distance) {
    double dist = getDistance(lat, lon);

    *distance = String(dist);

    if (dist > MAX_DISTANCE) return true;

    return false;
}
