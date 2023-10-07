#include <Arduino.h>

const float MAX_DISTANCE = 25; // Distancia límite utilizada para activar el envío de alerta de distancia, medida en metros

float LATITUDE_REFERENCE = 0;
float LONGITUDE_REFERENCE = 0;

/*
double getDistance(float lat, float lon)
{
    double dist = 60 * ((acos(sin(LATITUDE_REFERENCE * (PI / 180)) * sin(lat * (PI / 180)) +
                              cos(LATITUDE_REFERENCE * (PI / 180)) * cos(lat * (PI / 180)) *
                                  cos(abs((lon - LONGITUDE_REFERENCE)) * (PI / 180)))) *
                        (180 / PI));
    return dist * 1852;
}
*/
double getDistance(float lat1, float lon1)
{ // Radius of the Earth in kilometers (mean value)

    double R = 6371;
    // Convert latitude and longitude from degrees to radians

    double lat1Rad = (LATITUDE_REFERENCE * PI) / 180;

    double lon1Rad = (LONGITUDE_REFERENCE * PI) / 180;

    double lat2Rad = (lat1 * PI) / 180;

    double lon2Rad = (lon1 * PI) / 180;

    // Haversine formula

    double dLat = lat2Rad - lat1Rad;

    double dLon = lon2Rad - lon1Rad;

    double a =
        sin(dLat / 2) * sin(dLat / 2) +
        cos(lat1Rad) * cos(lat2Rad) * sin(dLon / 2) * sin(dLon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    double distance = R * c * 1000; // Convert to meters

    return distance; // Distance in meters
}

/* Verifica si el dispositivo ha superado el límite de distancia*/
bool deviceIsTooFar(float lat, float lon, String *distance)
{
    double dist = getDistance(lat, lon);

    *distance = String(dist);

    Serial.println("Dist : " + String(dist));
    if (dist > MAX_DISTANCE)
        return true;

    return false;
}
