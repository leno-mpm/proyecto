#include "mi_freertos_arduino.h"
#include "sensors.h"

// ==================== CONFIGURACIÓN WiFi Y RED ====================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// Configuración del Gateway (simularemos en Linux después)
const char* GATEWAY_IP = "192.168.1.100"; // IP de tu máquina Linux
const int GATEWAY_PORT = 8080;

// ==================== VARIABLES GLOBALES ====================
QueueHandle_t xDataQueue;
SemaphoreHandle_t xWifiMutex;

// Estructura para datos del sensor
typedef struct {
    char sensorType[20];
    float value;
    unsigned long timestamp;
} SensorData_t;

// ==================== TAREAS FreeRTOS ====================

// Tarea 1: Sensor de Temperatura
void vTaskTemperatureSensor(void *pvParameters) {
    (void)pvParameters;
    
    Serial.println("[TEMP] Tarea sensor temperatura iniciada");
    
    while(1) {
        float temperature = readVirtualTemperature();
        
        SensorData_t data;
        strcpy(data.sensorType, "temperature");
        data.value = temperature;
        data.timestamp = xTaskGetTickCount();
        
        // Enviar a cola para procesamiento
        if(xQueueSend(xDataQueue, &data, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            Serial.print("[TEMP] Enviado: ");
            Serial.print(temperature);
            Serial.println("°C");
        }
        
        vTaskDelay(2000 / portTICK_PERIOD_MS); // Lectura cada 2 segundos
    }
}

// Tarea 2: Sensor de Humedad
void vTaskHumiditySensor(void *pvParameters) {
    (void)pvParameters;
    
    Serial.println("[HUM] Tarea sensor humedad iniciada");
    
    while(1) {
        float humidity = readVirtualHumidity();
        
        SensorData_t data;
        strcpy(data.sensorType, "humidity");
        data.value = humidity;
        data.timestamp = xTaskGetTickCount();
        
        // Enviar a cola para procesamiento
        if(xQueueSend(xDataQueue, &data, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            Serial.print("[HUM] Enviado: ");
            Serial.print(humidity);
            Serial.println("%");
        }
        
        vTaskDelay(3000 / portTICK_PERIOD_MS); // Lectura cada 3 segundos
    }
}

// Tarea 3: Sensor de Calidad de Aire (simulado)
void vTaskAirQualitySensor(void *pvParameters) {
    (void)pvParameters;
    
    Serial.println("[AIR] Tarea sensor calidad aire iniciada");
    
    while(1) {
        float airQuality = readVirtualAirQuality();
        
        SensorData_t data;
        strcpy(data.sensorType, "air_quality");
        data.value = airQuality;
        data.timestamp = xTaskGetTickCount();
        
        // Enviar a cola para procesamiento
        if(xQueueSend(xDataQueue, &data, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            Serial.print("[AIR] Enviado: ");
            Serial.print(airQuality);
            Serial.println(" AQI");
        }
        
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Lectura cada 5 segundos
    }
}

// Tarea 4: Comunicación con Gateway
void vTaskGatewayCommunication(void *pvParameters) {
    (void)pvParameters;
    
    Serial.println("[GATEWAY] Tarea comunicación iniciada");
    
    while(1) {
        SensorData_t receivedData;
        
        // Recibir datos de la cola
        if(xQueueReceive(xDataQueue, &receivedData, 5000 / portTICK_PERIOD_MS) == pdTRUE) {
            
            // Tomar mutex para acceso seguro a WiFi
            if(xSemaphoreTake(xWifiMutex, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
                
                // Simular envío al Gateway
                Serial.print("[GATEWAY] Enviando al gateway: ");
                Serial.print(receivedData.sensorType);
                Serial.print(" = ");
                Serial.print(receivedData.value);
                Serial.print(" (tick: ");
                Serial.print(receivedData.timestamp);
                Serial.println(")");
                
                // Aquí iría el código real para enviar via WiFi
                // sendToGateway(receivedData);
                
                xSemaphoreGive(xWifiMutex);
            }
        }
        
        // Pequeño delay para no saturar
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Tarea 5: Monitor del Sistema
void vTaskSystemMonitor(void *pvParameters) {
    (void)pvParameters;
    
    Serial.println("[MONITOR] Tarea monitor sistema iniciada");
    unsigned long lastPrint = 0;
    
    while(1) {
        unsigned long currentTick = xTaskGetTickCount();
        
        // Imprimir estado cada 10 segundos
        if(currentTick - lastPrint >= 10000) {
            Serial.println("[MONITOR] === Estado del Sistema ===");
            Serial.print("[MONITOR] Ticks: ");
            Serial.println(currentTick);
            Serial.print("[MONITOR] Mensajes en cola: ");
            Serial.println(uxQueueMessagesWaiting(xDataQueue));
            Serial.println("[MONITOR] ===========================");
            lastPrint = currentTick;
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// ==================== SETUP Y CONFIGURACIÓN ====================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("     PUBLISHER IoT - ESP32/NodeMCU");
    Serial.println("     FreeRTOS desde Cero - V1.0");
    Serial.println("========================================");
    
    // Inicializar componentes
    initVirtualSensors();
    
    // Crear cola para datos de sensores
    xDataQueue = xQueueCreate(10, sizeof(SensorData_t));
    if(xDataQueue == NULL) {
        Serial.println("[ERROR] No se pudo crear la cola de datos");
        return;
    }
    
    // Crear mutex para WiFi
    xWifiMutex = xSemaphoreCreateMutex();
    if(xWifiMutex == NULL) {
        Serial.println("[ERROR] No se pudo crear el mutex WiFi");
        return;
    }
    
    // Simular conexión WiFi
    Serial.println("[WIFI] Conectando a WiFi...");
    delay(1000);
    Serial.println("[WIFI] Conectado a: Wokwi-GUEST");
    
    // Crear tareas FreeRTOS
    Serial.println("[FreeRTOS] Creando tareas...");
    
    if(xTaskCreate(vTaskTemperatureSensor, "TempSensor", 2048, NULL, 2, NULL) != pdTRUE) {
        Serial.println("[ERROR] No se pudo crear tarea Temperatura");
    }
    
    if(xTaskCreate(vTaskHumiditySensor, "HumSensor", 2048, NULL, 2, NULL) != pdTRUE) {
        Serial.println("[ERROR] No se pudo crear tarea Humedad");
    }
    
    if(xTaskCreate(vTaskAirQualitySensor, "AirSensor", 2048, NULL, 1, NULL) != pdTRUE) {
        Serial.println("[ERROR] No se pudo crear tarea Calidad Aire");
    }
    
    if(xTaskCreate(vTaskGatewayCommunication, "GatewayComm", 4096, NULL, 3, NULL) != pdTRUE) {
        Serial.println("[ERROR] No se pudo crear tarea Comunicación");
    }
    
    if(xTaskCreate(vTaskSystemMonitor, "SystemMonitor", 2048, NULL, 1, NULL) != pdTRUE) {
        Serial.println("[ERROR] No se pudo crear tarea Monitor");
    }
    
    Serial.println("[FreeRTOS] Todas las tareas creadas exitosamente");
    Serial.println("[SYSTEM] Sistema IoT Publisher INICIADO");
    Serial.println("========================================");
}

void loop() {
    // En FreeRTOS, el loop principal puede estar vacío
    // o usarse para tareas de baja prioridad
    delay(10000); // Pequeña tarea de fondo cada 10 segundos
}
