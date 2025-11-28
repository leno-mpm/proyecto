#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// ==================== SENSORES VIRTUALES ====================

class VirtualSensor {
private:
    float baseValue;
    float variation;
    float currentValue;
    unsigned long lastUpdate;
    
public:
    VirtualSensor(float base, float var) : baseValue(base), variation(var), currentValue(base), lastUpdate(0) {}
    
    float read() {
        unsigned long currentTime = millis();
        if(currentTime - lastUpdate > 100) { // Actualizar cada 100ms
            // Simular variación natural del sensor
            float randomChange = (random(-100, 100) / 100.0) * variation;
            currentValue = baseValue + randomChange;
            
            // Mantener dentro de rangos razonables
            if(baseValue == 25.0) { // Temperatura
                currentValue = constrain(currentValue, 15.0, 35.0);
            } else if(baseValue == 60.0) { // Humedad
                currentValue = constrain(currentValue, 40.0, 80.0);
            } else { // Calidad aire
                currentValue = constrain(currentValue, 0.0, 100.0);
            }
            
            lastUpdate = currentTime;
        }
        return currentValue;
    }
};

// Instancias globales de sensores virtuales
VirtualSensor tempSensor(25.0, 5.0);    // 25°C ±5°C
VirtualSensor humSensor(60.0, 15.0);    // 60% ±15%
VirtualSensor airSensor(85.0, 10.0);    // 85 AQI ±10

// Funciones de lectura de sensores
float readVirtualTemperature() {
    return tempSensor.read();
}

float readVirtualHumidity() {
    return humSensor.read();
}

float readVirtualAirQuality() {
    return airSensor.read();
}

void initVirtualSensors() {
    randomSeed(analogRead(0)); // Semilla para random
    Serial.println("[SENSORS] Sensores virtuales inicializados");
}

#endif
