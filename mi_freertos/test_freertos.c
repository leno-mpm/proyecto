#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mi_freertos.h"

// ============================================================
//               TAREAS DE PRUEBA (SENSORES Y CONTROL)
// ============================================================

void vTaskTemperatura(void *pvParameters) {
    (void)pvParameters;

    printf("[Temp] Iniciando sensor de temperatura...\n");

    for(int i = 0; i < 5; i++) {
        float temp = 25.0 + (rand() % 100) / 10.0;
        printf("[Temp] Lectura %d: %.1f°C, Tick=%lu\n",
               i+1, temp, xTaskGetTickCount());
        vTaskDelay(1000); // 1 segundo
    }

    printf("[Temp] Finalizado.\n");
}

void vTaskHumedad(void *pvParameters) {
    (void)pvParameters;

    printf("[Hum] Iniciando sensor de humedad...\n");

    for(int i = 0; i < 5; i++) {
        float hum = 60.0 + (rand() % 200) / 10.0;
        printf("[Hum] Lectura %d: %.1f%%, Tick=%lu\n",
               i+1, hum, xTaskGetTickCount());
        vTaskDelay(1500); // 1.5 segundos
    }

    printf("[Hum] Finalizado.\n");
}

void vTaskControl(void *pvParameters) {
    (void)pvParameters;

    printf("[Control] Sistema de control iniciado...\n");

    for(int i = 0; i < 3; i++) {
        printf("[Control] Ejecutando ciclo %d, Tick=%lu\n",
               i+1, xTaskGetTickCount());
        vTaskDelay(2000); // 2 segundos
    }

    printf("[Control] Finalizado.\n");
}

void vTaskMonitor(void *pvParameters) {
    (void)pvParameters;

    printf("[Monitor] Iniciando monitor de actividad...\n");

    for(int i = 0; i < 8; i++) {
        printf("[Monitor] Ping %d, Tick=%lu\n",
               i+1, xTaskGetTickCount());
        vTaskDelay(500); // 0.5 segundos
    }

    printf("[Monitor] Finalizado.\n");
}

// ============================================================
//                       PROGRAMA PRINCIPAL
// ============================================================

int main() {

    printf("\n===========================================\n");
    printf("     MI FREERTOS — SIMULACIÓN COMPLETA\n");
    printf("===========================================\n\n");

    srand(time(NULL));

    printf("FASE 1 — Creación de tareas...\n");

    xTaskCreate(vTaskTemperatura, "Temp",    1024, NULL, 1, NULL);
    xTaskCreate(vTaskHumedad,    "Hum",     1024, NULL, 1, NULL);
    xTaskCreate(vTaskControl,    "Control",  512, NULL, 2, NULL);
    xTaskCreate(vTaskMonitor,    "Monitor",  512, NULL, 3, NULL);

    printf("\nFASE 2 — Iniciando scheduler...\n");
    printf("===========================================\n\n");

    vTaskStartScheduler();

    printf("\n===========================================\n");
    printf("           SISTEMA COMPLETADO\n");
    printf("===========================================\n\n");

    return 0;
}

